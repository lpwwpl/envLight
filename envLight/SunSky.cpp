#include "SunSky.hpp"
#include <cmath>
#include <algorithm>

namespace SSLib {

    // ---------- 常量 ----------
    constexpr float PI = 3.14159265358979323846f;
    constexpr float DEG2RAD = PI / 180.0f;
    constexpr float RAD2DEG = 180.0f / PI;

    // ---------- 标准天空系数表 (15 种，0 基索引) ----------
    static const float kCoeffs[15][5] = {
        // A       B       C       D       E
        { 4.000f, -0.700f,  0.000f, -1.000f,  0.000f }, // 0  CIE Standard Overcast Sky
        { 4.000f, -0.700f,  2.000f, -0.800f,  0.000f }, // 1  Overcast steep grade some sun
        { 2.500f, -0.500f,  1.500f, -0.900f,  0.000f }, // 2  Overcast moderate grade no sun
        { 2.500f, -0.500f,  0.800f, -0.800f,  0.000f }, // 3  Overcast moderate grade some sun
        { 1.100f, -0.800f,  0.000f, -0.500f,  0.000f }, // 4  CIE Standard Uniform Sky
        { 0.000f, -0.500f,  1.500f, -0.600f,  0.000f }, // 5  Partly cloudy no grade some sun
        { 1.000f, -0.500f,  1.200f, -0.600f,  0.000f }, // 6  Partly cloudy no grade some sun
        { 0.500f, -0.300f,  2.500f, -0.600f,  0.000f }, // 7  Partly cloudy no grade distinct corona
        { 0.500f, -0.300f,  1.000f, -0.500f,  0.000f }, // 8  Partly cloudy obscured sun
        { 0.500f, -0.300f,  5.000f, -0.500f,  0.000f }, // 9  Partly cloudy circumscalar region
        { 1.100f, -0.400f,  2.000f, -0.700f,  0.000f }, // 10 White-blue sky distinct corona
        { 1.100f, -0.400f,  4.500f, -0.800f,  0.000f }, // 11 CIE Standard Clear Sky low turbidity
        { 1.100f, -0.500f,  4.500f, -0.800f,  0.000f }, // 12 CIE Standard Clear Sky some pollution
        { 0.500f, -0.500f,  6.000f, -0.700f,  0.000f }, // 13 Cloudless turbid sky broad corona
        { 0.500f, -0.500f,  6.000f, -0.600f,  0.000f }  // 14 White-blue turbid sky broad corona
    };

    // ---------- Perez 相对亮度函数 ----------
    static float CIELumRatio(const Vec3f& direction, const Vec3f& toSun,
        float a, float b, float c, float d, float e)
    {
        const float cosTheta = std::max(0.001f, direction.v[2]);
        const float cosThetaZ = std::max(0.001f, toSun.v[2]);

        // 天空点与太阳夹角 gamma
        const float cosGamma = std::max(-1.0f, std::min(1.0f, direction.dot(toSun)));
        const float gamma = std::acos(cosGamma);

        const float numerator = (1.0f + a * std::exp(b / cosTheta))
            * (1.0f + c * std::exp(d * gamma) + e * cosGamma * cosGamma);

        const float denominator = (1.0f + a * std::exp(b))
            * (1.0f + c * std::exp(d * std::acos(cosThetaZ)) + e * cosThetaZ * cosThetaZ);

        return numerator / denominator;
    }

    // ---------- 太阳位置计算 ----------
    Vec3f SunDirection(float decimalHour, float timeZone, int dayOfYear,
        float latitudeDeg, float longitudeDeg)
    {
        // 简化太阳位置算法（精度约 1°）
        const float latRad = latitudeDeg * DEG2RAD;

        // 太阳赤纬 (度)
        const float declination = 23.44f * std::sin((284 + dayOfYear) * 360.0f / 365.0f * DEG2RAD);

        // 时角 (度)
        const float hourAngle = (decimalHour - 12.0f) * 15.0f;

        const float decRad = declination * DEG2RAD;
        const float haRad = hourAngle * DEG2RAD;

        const float sinAlt = std::sin(latRad) * std::sin(decRad)
            + std::cos(latRad) * std::cos(decRad) * std::cos(haRad);
        const float altitude = std::asin(std::max(-1.0f, std::min(1.0f, sinAlt)));

        const float cosAz = (std::sin(decRad) - std::sin(latRad) * sinAlt)
            / (std::cos(latRad) * std::cos(altitude));
        float azimuth = std::acos(std::max(-1.0f, std::min(1.0f, cosAz)));
        if (std::sin(haRad) > 0.0f) azimuth = -azimuth; // 下午为负

        // 坐标系：X 东，Y 北，Z 天顶
        const float cosAlt = std::cos(altitude);
        const Vec3f dir = {
            cosAlt * std::sin(azimuth),
            cosAlt * std::cos(azimuth),
            std::sin(altitude)
        };
        return dir;
    }

    // ---------- 标准天空 ----------
    float CIEStandardSky(int type, const Vec3f& direction, const Vec3f& toSun, float zenithValue)
    {
        if (type < 0 || type >= 15) type = 0;
        const float* coeff = kCoeffs[type];
        return CIELumRatio(direction, toSun, coeff[0], coeff[1], coeff[2], coeff[3], coeff[4]) * zenithValue;
    }

    // ---------- 系数提取 ----------
    CIESkyCoefficients CIEStandardSkyCoefficients(int type)
    {
        if (type < 0 || type >= 15) type = 0;
        const float* coeff = kCoeffs[type];
        return { coeff[0], coeff[1], coeff[2], coeff[3], coeff[4] };
    }

    // ---------- 自定义系数天空 ----------
    float CIECustomSky(const CIESkyCoefficients& coeff, const Vec3f& direction,
        const Vec3f& toSun, float zenithValue)
    {
        return CIELumRatio(direction, toSun, coeff.a, coeff.b, coeff.c, coeff.d, coeff.e) * zenithValue;
    }

} // namespace SSLib