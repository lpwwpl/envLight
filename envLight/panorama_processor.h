#ifndef PANORAMA_PROCESSOR_H
#define PANORAMA_PROCESSOR_H

#include <string>
#include <vector>
#include <cstdint>
class QPointF;
struct sRGB {
    double r, g, b;
    sRGB(double r_ = 0.0, double g_ = 0.0, double b_ = 0.0) : r(r_), g(g_), b(b_) {}

    sRGB operator*(double s) const { return sRGB(r * s, g * s, b * s); }
    sRGB operator+(const sRGB& other) const { return sRGB(r + other.r, g + other.g, b + other.b); }
};

struct Image {
    int width, height;
    std::vector<std::vector<sRGB>> data; // data[y][x]
    Image() : width(0), height(0) {}
    Image(int w, int h) : width(w), height(h) {
        data.resize(static_cast<size_t>(h), std::vector<sRGB>(static_cast<size_t>(w)));
    }
    sRGB& at(int x, int y) { return data[static_cast<size_t>(y)][static_cast<size_t>(x)]; }
    const sRGB& at(int x, int y) const { return data[static_cast<size_t>(y)][static_cast<size_t>(x)]; }
};

class PanoramaProcessor {
public:
    // 加载 LDR 图像 (JPG, PNG, BMP, TGA)
    static bool loadImageLDR(const std::string& filename, Image& img);
    // 加载 EXR 图像（自动处理常规/多部分/深度）
    static bool loadImageEXR(const std::string& filename, Image& img,
        float exposure = 1.0f, float gamma = 2.2f);
    // 加载任意支持的图像（根据扩展名自动选择）
    static bool loadImage(const std::string& filename, Image& img,
        float exposure = 1.0f, float gamma = 2.2f);
    // 生成透视视图（支持独立 HFOV / VFOV）
    static Image perspectiveFromPanorama(const Image& pano,
        double cx, double cy, double cz,
        double yaw_deg, double pitch_deg, double roll_deg,
        double hfov_deg, double vfov_deg,
        int outW, int outH, int aa);
    std::vector<QPointF> computeCornerUVs(
        double cx, double cy, double cz,
        double yaw_deg, double pitch_deg, double roll_deg,
        double hfov_deg, double vfov_deg,
        int outW, int outH);
    // 辅助：射线与单位球面求交（供外部使用）
    static bool raySphereIntersection(const double origin[3], const double dir[3],
        double& hit_u, double& hit_v);
};

#endif // PANORAMA_PROCESSOR_H