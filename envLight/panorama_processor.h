#ifndef PANORAMA_PROCESSOR_H
#define PANORAMA_PROCESSOR_H

#include <string>
#include <vector>
#include <cstdint>
#include "coordinate_system.h"
class QPointF;

// World-coordinate representation is declared in coordinate_system.h.

// 8-bit display image. RGB values are expected in [0, 255].
struct sRGB {
    double r, g, b;
    sRGB(double r_ = 0.0, double g_ = 0.0, double b_ = 0.0) : r(r_), g(g_), b(b_) {}

    sRGB operator*(double s) const { return sRGB(r * s, g * s, b * s); }
    sRGB operator+(const sRGB& other) const { return sRGB(r + other.r, g + other.g, b + other.b); }
};

struct Image {
    int width, height;
    std::vector<std::vector<sRGB>> data; // data[y][x], display-only 8-bit range
    Image() : width(0), height(0) {}
    Image(int w, int h) : width(w), height(h) {
        data.resize(static_cast<size_t>(h), std::vector<sRGB>(static_cast<size_t>(w)));
    }
    sRGB& at(int x, int y) { return data[static_cast<size_t>(y)][static_cast<size_t>(x)]; }
    const sRGB& at(int x, int y) const { return data[static_cast<size_t>(y)][static_cast<size_t>(x)]; }
};

// Scene-linear floating-point RGB. Values are intentionally NOT clamped to [0,1] or [0,255].
// HDR/EXR radiance ratios are preserved in this representation.
struct LinearRGB {
    float r, g, b;
    LinearRGB(float r_ = 0.0f, float g_ = 0.0f, float b_ = 0.0f) : r(r_), g(g_), b(b_) {}

    LinearRGB operator*(float s) const { return LinearRGB(r * s, g * s, b * s); }
    LinearRGB operator+(const LinearRGB& other) const { return LinearRGB(r + other.r, g + other.g, b + other.b); }
};

struct HDRImage {
    int width, height;
    std::vector<LinearRGB> data; // contiguous row-major scene-linear pixels

    HDRImage() : width(0), height(0) {}
    HDRImage(int w, int h)
        : width(w), height(h),
          data(static_cast<size_t>(w) * static_cast<size_t>(h)) {}

    LinearRGB& at(int x, int y) {
        return data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
    }
    const LinearRGB& at(int x, int y) const {
        return data[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
    }
};

class PanoramaProcessor {
public:
    // Load all supported formats into a scene-linear floating-point image.
    // HDR/EXR values remain unclamped. LDR formats are decoded from sRGB to linear [0,1].
    static bool loadImageHDR(const std::string& filename, HDRImage& img);
    static bool loadImageLDR(const std::string& filename, HDRImage& img);
    static bool loadImageEXR(const std::string& filename, HDRImage& img);
    static bool loadImage(const std::string& filename, HDRImage& img);

    // Build camera-to-world rotation for navigation angles.
    // Geographic meaning is independent of the selected coordinate representation:
    // azimuth: 0=N, 90=E, 180=S, 270=W; elevation: positive looks Up.
    // ENU: X=East, Y=North, Z=Up. NED: X=North, Y=East, Z=Down.
    // Camera-local axes are +X=right, +Y=down, +Z=forward.
    static void buildNavigationRotation(double azimuth_deg, double elevation_deg, double roll_deg,
        double R[3][3], WorldCoordinateSystem coordinateSystem = WorldCoordinateSystem::ENU);

    // Convert a direction/position vector between ENU and NED. The conversion is its own inverse:
    // (E,N,U) <-> (N,E,D) = (y,x,-z).
    static void convertCoordinateVector(const double in[3], WorldCoordinateSystem from,
        WorldCoordinateSystem to, double out[3]);

    // Generate a perspective view while preserving HDR floating-point values.
    static HDRImage perspectiveFromPanorama(const HDRImage& pano,
        double cx, double cy, double cz,
        double yaw_deg, double pitch_deg, double roll_deg,
        double hfov_deg, double vfov_deg,
        int outW, int outH, int aa,
        double northPanoramaDeg = 180.0,
        WorldCoordinateSystem coordinateSystem = WorldCoordinateSystem::ENU);

    // Display-only conversion: tone map scene-linear HDR to 8-bit sRGB.
    // This never modifies the source HDR image.
    static Image toneMapForDisplay(const HDRImage& src,
        float exposure = 1.0f, float gamma = 2.2f);

    std::vector<QPointF> computeCornerUVs(
        double cx, double cy, double cz,
        double yaw_deg, double pitch_deg, double roll_deg,
        double hfov_deg, double vfov_deg,
        int outW, int outH,
        double northPanoramaDeg = 180.0,
        WorldCoordinateSystem coordinateSystem = WorldCoordinateSystem::ENU);

    // Helper: ray intersection with unit sphere.
    static bool raySphereIntersection(const double origin[3], const double dir[3],
        double& hit_u, double& hit_v,
        WorldCoordinateSystem coordinateSystem = WorldCoordinateSystem::ENU);
};

#endif // PANORAMA_PROCESSOR_H
