#include "panorama_processor.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 定义STB_IMAGE实现
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

#ifdef _WIN32
#include <windows.h> // for MultiByteToWideChar
#else
#include <cstdio>
#endif
#include <QPointF>
#include <optional>
// ----------------------------------------------------------------------
// 内部辅助函数（匿名命名空间）
// ----------------------------------------------------------------------
namespace {

    // 将 HDR 浮点数据转换为 8-bit sRGB (Reinhard + Gamma)
    void hdrToLDR(const float* rgba, int width, int height, Image& out,
        float exposure, float gamma) {
        out = Image(width, height);
        int pixelCount = width * height;
        float maxLum = 0.0f;
        for (int i = 0; i < pixelCount; ++i) {
            float r = rgba[4 * i];
            float g = rgba[4 * i + 1];
            float b = rgba[4 * i + 2];
            float lum = std::max({ r, g, b });
            if (lum > maxLum) maxLum = lum;
        }
        if (maxLum < 1e-6f) maxLum = 1.0f;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;
                float r = rgba[4 * idx] * exposure;
                float g = rgba[4 * idx + 1] * exposure;
                float b = rgba[4 * idx + 2] * exposure;
                r = r / (1.0f + r);
                g = g / (1.0f + g);
                b = b / (1.0f + b);
                float invGamma = 1.0f / gamma;
                r = std::pow(r, invGamma);
                g = std::pow(g, invGamma);
                b = std::pow(b, invGamma);
                auto toUint8 = [](float v) -> uint8_t {
                    v = std::max(0.0f, std::min(1.0f, v));
                    return static_cast<uint8_t>(v * 255.0f + 0.5f);
                };
                out.at(x, y) = sRGB(toUint8(r), toUint8(g), toUint8(b));
            }
        }
    }

    // 常规 EXR 加载
    bool loadEXR_Regular(const std::string& filename, Image& img,
        float exposure, float gamma) {
        float* rgba = nullptr;
        int width, height;
        const char* err = nullptr;
        int ret = LoadEXR(&rgba, &width, &height, filename.c_str(), &err);
        if (ret == TINYEXR_SUCCESS) {
            hdrToLDR(rgba, width, height, img, exposure, gamma);
            free(rgba);
            return true;
        }
        if (err) FreeEXRErrorMessage(err);
        return false;
    }

    // 多部分 EXR 加载
    bool loadEXR_MultiPart(const std::string& filename, Image& img,
        float exposure, float gamma) {
        EXRVersion version;
        if (ParseEXRVersionFromFile(&version, filename.c_str()) != TINYEXR_SUCCESS)
            return false;

        EXRHeader** headers = nullptr;
        int num_headers;
        const char* err = nullptr;
        if (ParseEXRMultipartHeaderFromFile(&headers, &num_headers, &version,
            filename.c_str(), &err) != TINYEXR_SUCCESS) {
            if (err) FreeEXRErrorMessage(err);
            return false;
        }

        int selected_part = -1;
        for (int i = 0; i < num_headers; ++i) {
            EXRHeader* hdr = headers[i];
            bool hasR = false, hasG = false, hasB = false;
            for (int c = 0; c < hdr->num_channels; ++c) {
                std::string ch_name(hdr->channels[c].name);
                if (ch_name == "R" || ch_name == "r") hasR = true;
                if (ch_name == "G" || ch_name == "g") hasG = true;
                if (ch_name == "B" || ch_name == "b") hasB = true;
            }
            if (hasR && hasG && hasB) { selected_part = i; break; }
        }
        if (selected_part == -1) selected_part = 0;

        for (int i = 0; i < num_headers; ++i) {
            EXRHeader* hdr = headers[i];
            for (int c = 0; c < hdr->num_channels; ++c)
                hdr->requested_pixel_types[c] = TINYEXR_PIXELTYPE_FLOAT;
        }

        EXRImage* images = new EXRImage[num_headers];
        for (int i = 0; i < num_headers; ++i) InitEXRImage(&images[i]);
        int ret = LoadEXRMultipartImageFromFile(images, (const EXRHeader**)headers,
            num_headers, filename.c_str(), &err);
        if (ret != TINYEXR_SUCCESS) {
            if (err) FreeEXRErrorMessage(err);
            for (int i = 0; i < num_headers; ++i) FreeEXRHeader(headers[i]);
            free(headers);
            delete[] images;
            return false;
        }

        EXRImage& imgPart = images[selected_part];
        int width = imgPart.width, height = imgPart.height;
        EXRHeader* selHdr = headers[selected_part];
        int idxR = -1, idxG = -1, idxB = -1;
        for (int c = 0; c < selHdr->num_channels; ++c) {
            std::string ch_name(selHdr->channels[c].name);
            if (ch_name == "R" || ch_name == "r") idxR = c;
            if (ch_name == "G" || ch_name == "g") idxG = c;
            if (ch_name == "B" || ch_name == "b") idxB = c;
        }
        if (idxR == -1 || idxG == -1 || idxB == -1) {
            for (int i = 0; i < num_headers; ++i) FreeEXRImage(&images[i]);
            for (int i = 0; i < num_headers; ++i) FreeEXRHeader(headers[i]);
            free(headers);
            delete[] images;
            return false;
        }

        std::vector<float> rgba(width * height * 4, 0.0f);
        float** imgPtrs = (float**)imgPart.images;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;
                rgba[4 * idx + 0] = imgPtrs[idxR][idx];
                rgba[4 * idx + 1] = imgPtrs[idxG][idx];
                rgba[4 * idx + 2] = imgPtrs[idxB][idx];
                rgba[4 * idx + 3] = 1.0f;
            }
        }
        hdrToLDR(rgba.data(), width, height, img, exposure, gamma);

        for (int i = 0; i < num_headers; ++i) FreeEXRImage(&images[i]);
        for (int i = 0; i < num_headers; ++i) FreeEXRHeader(headers[i]);
        free(headers);
        delete[] images;
        return true;
    }

    // 深度 EXR 加载
    bool loadEXR_Deep(const std::string& filename, Image& img,
        float exposure, float gamma) {
        DeepImage deep;
        const char* err = nullptr;
        int ret = LoadDeepEXR(&deep, filename.c_str(), &err);
        if (ret != TINYEXR_SUCCESS) {
            if (err) FreeEXRErrorMessage(err);
            return false;
        }

        int idxR = -1, idxG = -1, idxB = -1;
        for (int c = 0; c < deep.num_channels; ++c) {
            std::string ch_name(deep.channel_names[c]);
            if (ch_name == "R" || ch_name == "r") idxR = c;
            if (ch_name == "G" || ch_name == "g") idxG = c;
            if (ch_name == "B" || ch_name == "b") idxB = c;
        }
        if (idxR == -1 || idxG == -1 || idxB == -1) {
            for (int c = 0; c < deep.num_channels; ++c) {
                for (int y = 0; y < deep.height; ++y) free(deep.image[c][y]);
                free(deep.image[c]);
                free((void*)deep.channel_names[c]);
            }
            free(deep.image);
            for (int y = 0; y < deep.height; ++y) free(deep.offset_table[y]);
            free(deep.offset_table);
            return false;
        }

        int width = deep.width, height = deep.height;
        std::vector<float> rgba(width * height * 4, 0.0f);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int sampleCount = deep.offset_table[y][x];
                int idx = y * width + x;
                if (sampleCount > 0) {
                    rgba[4 * idx + 0] = deep.image[idxR][y][0];
                    rgba[4 * idx + 1] = deep.image[idxG][y][0];
                    rgba[4 * idx + 2] = deep.image[idxB][y][0];
                    rgba[4 * idx + 3] = 1.0f;
                }
                else {
                    rgba[4 * idx + 0] = 0.0f;
                    rgba[4 * idx + 1] = 0.0f;
                    rgba[4 * idx + 2] = 0.0f;
                    rgba[4 * idx + 3] = 1.0f;
                }
            }
        }
        hdrToLDR(rgba.data(), width, height, img, exposure, gamma);

        for (int c = 0; c < deep.num_channels; ++c) {
            for (int y = 0; y < deep.height; ++y) free(deep.image[c][y]);
            free(deep.image[c]);
            free((void*)deep.channel_names[c]);
        }
        free(deep.image);
        for (int y = 0; y < deep.height; ++y) free(deep.offset_table[y]);
        free(deep.offset_table);
        return true;
    }

    // 射线与单位球面求交
    bool raySphereIntersection(const double origin[3], const double dir[3],
        double& hit_u, double& hit_v) {
        double a = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
        double b = 2.0 * (origin[0] * dir[0] + origin[1] * dir[1] + origin[2] * dir[2]);
        double c = origin[0] * origin[0] + origin[1] * origin[1] + origin[2] * origin[2] - 1.0;
        double disc = b * b - 4.0 * a * c;
        if (disc < 0.0) return false;
        double sqrt_disc = sqrt(disc);
        double t1 = (-b - sqrt_disc) / (2.0 * a);
        double t2 = (-b + sqrt_disc) / (2.0 * a);
        double t = (t1 > 1e-6) ? t1 : ((t2 > 1e-6) ? t2 : -1.0);
        if (t <= 1e-6) return false;
        double hit_x = origin[0] + t * dir[0];
        double hit_y = origin[1] + t * dir[1];
        double hit_z = origin[2] + t * dir[2];
        double theta = acos(hit_y);
        double phi = atan2(hit_x, hit_z);
        hit_u = (phi + M_PI) / (2.0 * M_PI);
        hit_v = theta / M_PI;
        return true;
    }

    // 双线性采样
    sRGB samplePanoramaBilinear(const Image& pano, double u, double v) {
        double px = u * (pano.width - 1);
        double py = v * (pano.height - 1);
        int x0 = static_cast<int>(px);
        int y0 = static_cast<int>(py);
        int x1 = x0 + 1;
        int y1 = y0 + 1;
        x0 = (x0 % pano.width + pano.width) % pano.width;
        x1 = (x1 % pano.width + pano.width) % pano.width;
        y0 = std::max(0, std::min(y0, pano.height - 1));
        y1 = std::max(0, std::min(y1, pano.height - 1));
        double dx = px - x0;
        double dy = py - y0;
        const sRGB& c00 = pano.at(x0, y0);
        const sRGB& c10 = pano.at(x1, y0);
        const sRGB& c01 = pano.at(x0, y1);
        const sRGB& c11 = pano.at(x1, y1);
        double r = (1 - dx) * (1 - dy) * c00.r + dx * (1 - dy) * c10.r + (1 - dx) * dy * c01.r + dx * dy * c11.r;
        double g = (1 - dx) * (1 - dy) * c00.g + dx * (1 - dy) * c10.g + (1 - dx) * dy * c01.g + dx * dy * c11.g;
        double b = (1 - dx) * (1 - dy) * c00.b + dx * (1 - dy) * c10.b + (1 - dx) * dy * c01.b + dx * dy * c11.b;
        return sRGB(static_cast<uint8_t>(r + 0.5), static_cast<uint8_t>(g + 0.5), static_cast<uint8_t>(b + 0.5));
    }

} // namespace anonymous

// ----------------------------------------------------------------------
// PanoramaProcessor 静态方法实现
// ----------------------------------------------------------------------
bool PanoramaProcessor::loadImageLDR(const std::string& filename, Image& img) {
#ifdef _WIN32
    // 解决中文路径问题：使用 _wfopen
    int wlen = MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, nullptr, 0);
    std::vector<wchar_t> wfilename(wlen);
    MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, wfilename.data(), wlen);
    FILE* file = _wfopen(wfilename.data(), L"rb");
#else
    FILE* file = fopen(filename.c_str(), "rb");
#endif
    if (!file) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return false;
    }
    int w, h, channels;
    unsigned char* data = stbi_load_from_file(file, &w, &h, &channels, 3);
    fclose(file);
    if (!data) {
        std::cerr << "stb_image 解码失败: " << stbi_failure_reason() << std::endl;
        return false;
    }
    img = Image(w, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = (y * w + x) * 3;
            img.at(x, y) = sRGB(data[idx], data[idx + 1], data[idx + 2]);
        }
    }
    stbi_image_free(data);
    return true;
}

bool PanoramaProcessor::loadImageEXR(const std::string& filename, Image& img,
    float exposure, float gamma) {
    if (loadEXR_Regular(filename, img, exposure, gamma)) return true;
    if (loadEXR_MultiPart(filename, img, exposure, gamma)) return true;
    if (loadEXR_Deep(filename, img, exposure, gamma)) return true;
    return false;
}

bool PanoramaProcessor::loadImage(const std::string& filename, Image& img,
    float exposure, float gamma) {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == "exr") {
        return loadImageEXR(filename, img, exposure, gamma);
    }
    else if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "bmp" || ext == "tga") {
        return loadImageLDR(filename, img);
    }
    return false;
}

bool PanoramaProcessor::raySphereIntersection(const double origin[3], const double dir[3],
    double& hit_u, double& hit_v) {
    return ::raySphereIntersection(origin, dir, hit_u, hit_v);
}
std::vector<QPointF> PanoramaProcessor::computeCornerUVs(
    double cx, double cy, double cz,
    double yaw_deg, double pitch_deg, double roll_deg,
    double hfov_deg, double vfov_deg,
    int outW, int outH)
{
    std::vector<QPointF> polygon;
    polygon.reserve(200); // 预分配

    // 旋转矩阵（与 perspectiveFromPanorama 完全一致）
    double yaw = yaw_deg * M_PI / 180.0;
    double pitch = pitch_deg * M_PI / 180.0;
    double roll = roll_deg * M_PI / 180.0;
    double cyaw = cos(yaw), syaw = sin(yaw);
    double cp = cos(pitch), sp = sin(pitch);
    double cr = cos(roll), sr = sin(roll);
    double R[3][3] = {
        { cyaw * cp,  cyaw * sp * sr - syaw * cr,  cyaw * sp * cr + syaw * sr },
        { syaw * cp,  syaw * sp * sr + cyaw * cr,  syaw * sp * cr - cyaw * sr },
        { -sp,        cp * sr,                      cp * cr }
    };

    // 焦距
    double hfov_rad = hfov_deg * M_PI / 180.0;
    double focalX = (outW / 2.0) / tan(hfov_rad / 2.0);
    double focalY;
    if (vfov_deg > 0.0) {
        double vfov_rad = vfov_deg * M_PI / 180.0;
        focalY = (outH / 2.0) / tan(vfov_rad / 2.0);
    }
    else {
        focalY = focalX * (static_cast<double>(outH) / outW);
    }
    double halfW = outW / 2.0;
    double halfH = outH / 2.0;
    double origin[3] = { cx, cy, cz };

    // 采样辅助函数
    auto samplePoint = [&](double x, double y) -> std::optional<QPointF> {
        double nx = (x - halfW) / focalX;
        double ny = (y - halfH) / focalY;
        double nz = 1.0;
        double len = sqrt(nx * nx + ny * ny + nz * nz);
        if (len < 1e-6) return std::nullopt;
        nx /= len; ny /= len; nz /= len;

        double wx = R[0][0] * nx + R[0][1] * ny + R[0][2] * nz;
        double wy = R[1][0] * nx + R[1][1] * ny + R[1][2] * nz;
        double wz = R[2][0] * nx + R[2][1] * ny + R[2][2] * nz;
        len = sqrt(wx * wx + wy * wy + wz * wz);
        if (len < 1e-6) return std::nullopt;
        wx /= len; wy /= len; wz /= len;

        double dir[3] = { wx, wy, wz };
        double u, v;
        if (raySphereIntersection(origin, dir, u, v)) {
            // 注意：这里不添加镜像，保持与 perspectiveFromPanorama 一致
            // 如果您的全景图需要镜像，请在此处调整 u, v
            return QPointF(u, v);
        }
        return std::nullopt;
    };

    const int N = 40; // 每条边采样点数（越高越精确，但计算量稍大）

    // 采样上边缘 (y=0)
    for (int i = 0; i <= N; ++i) {
        double x = i * (outW - 1.0) / N;
        auto pt = samplePoint(x, 0.0);
        if (pt) polygon.push_back(*pt);
    }
    // 右边缘 (x=outW-1)
    for (int i = 1; i <= N; ++i) {
        double y = i * (outH - 1.0) / N;
        auto pt = samplePoint(outW - 1.0, y);
        if (pt) polygon.push_back(*pt);
    }
    // 下边缘 (y=outH-1)
    for (int i = N - 1; i >= 0; --i) {
        double x = i * (outW - 1.0) / N;
        auto pt = samplePoint(x, outH - 1.0);
        if (pt) polygon.push_back(*pt);
    }
    // 左边缘 (x=0)
    for (int i = N - 1; i >= 1; --i) {
        double y = i * (outH - 1.0) / N;
        auto pt = samplePoint(0.0, y);
        if (pt) polygon.push_back(*pt);
    }

    // 如果采样点太少（例如全部无交点），返回空
    if (polygon.size() < 3) polygon.clear();
    return polygon;
}
Image PanoramaProcessor::perspectiveFromPanorama(const Image& pano,
    double cx, double cy, double cz,
    double yaw_deg, double pitch_deg, double roll_deg,
    double hfov_deg, double vfov_deg,
    int outW, int outH, int aa) {
    Image output(outW, outH);
    if (pano.width == 0 || pano.height == 0) return output;

    double cameraPos[3] = { cx, cy, cz };
    double yaw = yaw_deg * M_PI / 180.0;
    double pitch = pitch_deg * M_PI / 180.0;
    double roll = roll_deg * M_PI / 180.0;
    double cyaw = cos(yaw), syaw = sin(yaw);
    double cp = cos(pitch), sp = sin(pitch);
    double cr = cos(roll), sr = sin(roll);
    double R[3][3] = {
        { cyaw * cp,  cyaw * sp * sr - syaw * cr,  cyaw * sp * cr + syaw * sr },
        { syaw * cp,  syaw * sp * sr + cyaw * cr,  syaw * sp * cr - cyaw * sr },
        { -sp,      cp * sr,                  cp * cr }
    };

    double hfov_rad = hfov_deg * M_PI / 180.0;
    double focalX = (outW / 2.0) / tan(hfov_rad / 2.0);
    double focalY;
    if (vfov_deg > 0.0) {
        double vfov_rad = vfov_deg * M_PI / 180.0;
        focalY = (outH / 2.0) / tan(vfov_rad / 2.0);
    }
    else {
        focalY = focalX * (static_cast<double>(outH) / outW);
    }

    double halfW = outW / 2.0;
    double halfH = outH / 2.0;
    int samples = aa * aa;
    double step = 1.0 / aa;

    for (int y = 0; y < outH; ++y) {
        for (int x = 0; x < outW; ++x) {
            double r_sum = 0.0, g_sum = 0.0, b_sum = 0.0;
            int valid = 0;
            for (int sy = 0; sy < aa; ++sy) {
                for (int sx = 0; sx < aa; ++sx) {
                    double offX = (sx + 0.5) * step - 0.5;
                    double offY = (sy + 0.5) * step - 0.5;
                    double nx = (x + offX - halfW) / focalX;
                    double ny = (y + offY - halfH) / focalY;
                    double nz = 1.0;
                    double len = sqrt(nx * nx + ny * ny + nz * nz);
                    nx /= len; ny /= len; nz /= len;
                    double wx = R[0][0] * nx + R[0][1] * ny + R[0][2] * nz;
                    double wy = R[1][0] * nx + R[1][1] * ny + R[1][2] * nz;
                    double wz = R[2][0] * nx + R[2][1] * ny + R[2][2] * nz;
                    len = sqrt(wx * wx + wy * wy + wz * wz);
                    wx /= len; wy /= len; wz /= len;
                    double dir[3] = { wx, wy, wz };
                    double u, v;
                    if (raySphereIntersection(cameraPos, dir, u, v)) {
                        if (u >= 0 && u <= 1 && v >= 0 && v <= 1) {
                            sRGB col = samplePanoramaBilinear(pano, u, v);
                            r_sum += col.r; g_sum += col.g; b_sum += col.b;
                            valid++;
                        }
                    }
                }
            }
            if (valid > 0) {
                output.at(x, y) = sRGB(static_cast<uint8_t>(r_sum / valid + 0.5),
                    static_cast<uint8_t>(g_sum / valid + 0.5),
                    static_cast<uint8_t>(b_sum / valid + 0.5));
            }
            else {
                output.at(x, y) = sRGB(0, 0, 0);
            }
        }
    }
    return output;
}