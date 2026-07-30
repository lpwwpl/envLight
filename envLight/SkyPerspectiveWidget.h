#ifndef SKYPERSPECTIVEWIDGET_H
#define SKYPERSPECTIVEWIDGET_H

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QVector3D>
#include <QWidget>

#include "SunSky.hpp"

enum class SkyAbsoluteScaleMode {
    ZenithLuminance,              // cd/m2
    DiffuseHorizontalIlluminance, // lx
    DiffuseHorizontalIrradiance   // W/m2
};

enum class SkyColorMode {
    GrayscaleLuminance,
    FalseColor,
    NaturalPreview
};

enum class SkyToneMapMode {
    FixedReference,
    AutoPeak
};

struct SkyPerspectiveParameters {
    int cieSkyType = 11; // 0-based: CIE type 12
    bool customCoefficients = false;
    SSLib::CIESkyCoefficients coefficients;

    // Absolute calibration of the diffuse sky.
    SkyAbsoluteScaleMode scaleMode =
        SkyAbsoluteScaleMode::DiffuseHorizontalIrradiance;
    double targetValue = 100.0;

    // Direct normal irradiance or illuminance. Its unit must match scaleMode:
    // - Irradiance mode: W/m2
    // - Illuminance / luminance mode: lx
    double directNormalValue = 600.0;

    QVector3D sunDirection{0.5f, -0.5f, 0.7071f};

    // Camera: azimuth clockwise from North; pitch above horizon.
    double cameraAzimuthDeg = 180.0;
    double cameraPitchDeg = 20.0;
    double verticalFovDeg = 90.0;

    // Display-only controls. They never modify the physical sky values.
    SkyColorMode colorMode = SkyColorMode::NaturalPreview;
    SkyToneMapMode toneMapMode = SkyToneMapMode::FixedReference;
    double displayReferenceValue = 50.0;
    double exposure = 1.0;
    double gamma = 2.2;

    bool showHorizon = true;
    bool showSunDisk = true;
    bool showSunGlow = true;
    double sunAngularRadiusDeg = 0.2665;
};

class SkyPerspectiveWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit SkyPerspectiveWidget(QWidget* parent = nullptr);

    void setParameters(const SkyPerspectiveParameters& parameters);
    const SkyPerspectiveParameters& parameters() const;

    QImage renderToImage(const QSize& imageSize) const;
    bool savePng(const QString& filePath, const QSize& imageSize) const;

signals:
    void cameraChanged(
        double azimuthDeg,
        double pitchDeg,
        double verticalFovDeg);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void rebuildPreview();

    double relativeSkyValue(const QVector3D& direction) const;
    double absoluteScale() const;
    double toneMappedValue(double value, double referenceValue) const;
    double clearSkyFactor() const;

    QVector3D cameraRay(
        int x, int y, int width, int height) const;

    QColor naturalPreviewColor(
        const QVector3D& direction,
        double normalizedBrightness,
        double sunCosine,
        double directStrength) const;

    static QColor falseColor(double normalized);
    static double clamp(double value, double low, double high);
    static QVector3D mix(
        const QVector3D& a,
        const QVector3D& b,
        double t);

private:
    SkyPerspectiveParameters m_parameters;
    QImage m_preview;
    QPoint m_lastMousePosition;
};

#endif // SKYPERSPECTIVEWIDGET_H
