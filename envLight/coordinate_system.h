#ifndef ENVLIGHT_COORDINATE_SYSTEM_H
#define ENVLIGHT_COORDINATE_SYSTEM_H

#include <cmath>

// World-coordinate representation used only at scene / visualization boundaries.
// Physical sky, solar and weather calculations stay in canonical ENU.
enum class WorldCoordinateSystem {
    ENU = 0, // X=East,  Y=North, Z=Up
    NED = 1  // X=North, Y=East,  Z=Down
};

namespace CoordinateSystemUtils {

// ENU(E,N,U) <-> NED(N,E,D). This transform is its own inverse.
inline void convertVector(const double in[3], WorldCoordinateSystem from,
    WorldCoordinateSystem to, double out[3])
{
    if (from == to) {
        out[0] = in[0];
        out[1] = in[1];
        out[2] = in[2];
        return;
    }

    out[0] = in[1];
    out[1] = in[0];
    out[2] = -in[2];
}

// Canonical physical direction in ENU from navigation angles.
// Azimuth: clockwise from North (0=N, 90=E).
// Elevation: positive above horizon.
inline void directionENURadians(double azimuthRad, double elevationRad, double out[3])
{
    const double ce = std::cos(elevationRad);
    out[0] = ce * std::sin(azimuthRad); // East
    out[1] = ce * std::cos(azimuthRad); // North
    out[2] = std::sin(elevationRad);    // Up
}

inline void directionENU(double azimuthDeg, double elevationDeg, double out[3])
{
    const double pi = 3.14159265358979323846;
    directionENURadians(azimuthDeg * pi / 180.0, elevationDeg * pi / 180.0, out);
}

inline bool isENU(WorldCoordinateSystem system)
{
    return system == WorldCoordinateSystem::ENU;
}

} // namespace CoordinateSystemUtils

#endif // ENVLIGHT_COORDINATE_SYSTEM_H
