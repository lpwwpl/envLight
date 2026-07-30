#include "SkyPerspectiveWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QVector>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDegToRad = kPi / 180.0;

SSLib::Vec3f toSunSky(const QVector3D& value)
{
    return {value.x(), value.y(), value.z()};
}

} // namespace

SkyPerspectiveWidget::SkyPerspectiveWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(520, 340);
    setMouseTracking(true);
}

void SkyPerspectiveWidget::setParameters(
    const SkyPerspectiveParameters& parameters)
{
    m_parameters = parameters;

    m_parameters.cieSkyType =
        std::max(0, std::min(14, m_parameters.cieSkyType));

    while (m_parameters.cameraAzimuthDeg < 0.0)
        m_parameters.cameraAzimuthDeg += 360.0;
    while (m_parameters.cameraAzimuthDeg >= 360.0)
        m_parameters.cameraAzimuthDeg -= 360.0;

    m_parameters.cameraPitchDeg =
        clamp(m_parameters.cameraPitchDeg, -89.9, 89.9);
    m_parameters.verticalFovDeg =
        clamp(m_parameters.verticalFovDeg, 0, 179.9);

    m_parameters.targetValue =
        std::max(0.0, m_parameters.targetValue);
    m_parameters.directNormalValue =
        std::max(0.0, m_parameters.directNormalValue);
    m_parameters.displayReferenceValue =
        std::max(1.0e-9, m_parameters.displayReferenceValue);
    m_parameters.exposure =
        std::max(0.001, m_parameters.exposure);
    m_parameters.gamma =
        std::max(0.1, m_parameters.gamma);
    m_parameters.sunAngularRadiusDeg =
        clamp(m_parameters.sunAngularRadiusDeg, 0.05, 5.0);

    if (m_parameters.sunDirection.lengthSquared() < 1.0e-12f)
        m_parameters.sunDirection = QVector3D(0.0f, 0.0f, 1.0f);

    m_parameters.sunDirection.normalize();

    rebuildPreview();
    update();
}

const SkyPerspectiveParameters&
SkyPerspectiveWidget::parameters() const
{
    return m_parameters;
}

double SkyPerspectiveWidget::clamp(
    double value, double low, double high)
{
    return std::max(low, std::min(value, high));
}

QVector3D SkyPerspectiveWidget::mix(
    const QVector3D& a,
    const QVector3D& b,
    double t)
{
    const float tf = static_cast<float>(clamp(t, 0.0, 1.0));
    return a * (1.0f - tf) + b * tf;
}

double SkyPerspectiveWidget::relativeSkyValue(
    const QVector3D& direction) const
{
    if (direction.z() <= 0.0f)
        return 0.0;

    const SSLib::Vec3f sky =
        toSunSky(direction.normalized());
    const SSLib::Vec3f sun =
        toSunSky(m_parameters.sunDirection.normalized());

    double value = 0.0;

    if (m_parameters.customCoefficients) {
        value = SSLib::CIECustomSky(
            m_parameters.coefficients,
            sky,
            sun,
            1.0f);
    } else {
        value = SSLib::CIEStandardSky(
            m_parameters.cieSkyType,
            sky,
            sun,
            1.0f);
    }

    if (!std::isfinite(value))
        return 0.0;

    return std::max(0.0, value);
}

double SkyPerspectiveWidget::absoluteScale() const
{
    if (m_parameters.targetValue <= 0.0)
        return 0.0;

    if (m_parameters.scaleMode ==
        SkyAbsoluteScaleMode::ZenithLuminance) {

        const double zenithRelative =
            relativeSkyValue(
                QVector3D(0.0f, 0.0f, 1.0f));

        return zenithRelative > 1.0e-12
            ? m_parameters.targetValue / zenithRelative
            : 0.0;
    }

    // Horizontal diffuse quantity:
    // E_h = integral_hemisphere L(omega) cos(theta) dOmega.
    const int altitudeSteps = 72;
    const int azimuthSteps = 288;

    const double dAltitude =
        (0.5 * kPi) / altitudeSteps;
    const double dAzimuth =
        (2.0 * kPi) / azimuthSteps;

    double integral = 0.0;

    for (int altitudeIndex = 0;
         altitudeIndex < altitudeSteps;
         ++altitudeIndex) {

        const double altitude =
            (altitudeIndex + 0.5) * dAltitude;
        const double sinAltitude =
            std::sin(altitude);
        const double cosAltitude =
            std::cos(altitude);

        for (int azimuthIndex = 0;
             azimuthIndex < azimuthSteps;
             ++azimuthIndex) {

            const double azimuth =
                (azimuthIndex + 0.5) * dAzimuth;

            const QVector3D direction(
                static_cast<float>(
                    cosAltitude * std::sin(azimuth)),
                static_cast<float>(
                    cosAltitude * std::cos(azimuth)),
                static_cast<float>(sinAltitude));

            integral +=
                relativeSkyValue(direction)
                * sinAltitude
                * cosAltitude
                * dAltitude
                * dAzimuth;
        }
    }

    return integral > 1.0e-12
        ? m_parameters.targetValue / integral
        : 0.0;
}

double SkyPerspectiveWidget::toneMappedValue(
    double value,
    double referenceValue) const
{
    if (value <= 0.0 || referenceValue <= 0.0)
        return 0.0;

    const double mapped =
        1.0 - std::exp(
            -m_parameters.exposure
            * value
            / referenceValue);

    return clamp(
        std::pow(
            clamp(mapped, 0.0, 1.0),
            1.0 / m_parameters.gamma),
        0.0,
        1.0);
}

double SkyPerspectiveWidget::clearSkyFactor() const
{
    if (m_parameters.customCoefficients)
        return 0.55;

    const int type = m_parameters.cieSkyType + 1;

    if (type <= 4)
        return 0.08;
    if (type == 5)
        return 0.22;
    if (type <= 8)
        return 0.48;
    if (type <= 11)
        return 0.68;

    return 0.95;
}

QVector3D SkyPerspectiveWidget::cameraRay(
    int x,
    int y,
    int width,
    int height) const
{
    const double yaw =
        m_parameters.cameraAzimuthDeg * kDegToRad;
    const double pitch =
        m_parameters.cameraPitchDeg * kDegToRad;

    // ENU coordinates:
    // +X East, +Y North, +Z Zenith.
    const QVector3D forward(
        static_cast<float>(
            std::cos(pitch) * std::sin(yaw)),
        static_cast<float>(
            std::cos(pitch) * std::cos(yaw)),
        static_cast<float>(std::sin(pitch)));

    QVector3D right =
        QVector3D::crossProduct(
            forward,
            QVector3D(0.0f, 0.0f, 1.0f));

    if (right.lengthSquared() < 1.0e-10f)
        right = QVector3D(1.0f, 0.0f, 0.0f);
    else
        right.normalize();

    const QVector3D up =
        QVector3D::crossProduct(
            right,
            forward).normalized();

    const double aspect =
        static_cast<double>(width)
        / std::max(1, height);

    const double tanHalfFov =
        std::tan(
            0.5
            * m_parameters.verticalFovDeg
            * kDegToRad);

    const double screenX =
        (2.0 * (x + 0.5) / width - 1.0)
        * aspect
        * tanHalfFov;

    const double screenY =
        (1.0 - 2.0 * (y + 0.5) / height)
        * tanHalfFov;

    return (
        forward
        + static_cast<float>(screenX) * right
        + static_cast<float>(screenY) * up
    ).normalized();
}

QColor SkyPerspectiveWidget::falseColor(double value)
{
    const double t = clamp(value, 0.0, 1.0);

    const double r =
        clamp(
            1.5 - std::abs(4.0 * t - 3.0),
            0.0,
            1.0);

    const double g =
        clamp(
            1.5 - std::abs(4.0 * t - 2.0),
            0.0,
            1.0);

    const double b =
        clamp(
            1.5 - std::abs(4.0 * t - 1.0),
            0.0,
            1.0);

    return QColor::fromRgbF(r, g, b);
}

QColor SkyPerspectiveWidget::naturalPreviewColor(
    const QVector3D& direction,
    double normalizedBrightness,
    double sunCosine,
    double directStrength) const
{
    const double clarity = clearSkyFactor();
    const double altitudeFactor =
        std::pow(
            clamp(
                static_cast<double>(direction.z()),
                0.0,
                1.0),
            0.35);

    const QVector3D overcastHorizon(
        0.70f, 0.72f, 0.74f);
    const QVector3D overcastZenith(
        0.55f, 0.59f, 0.63f);

    const QVector3D clearHorizon(
        0.68f, 0.82f, 0.98f);
    const QVector3D clearZenith(
        0.10f, 0.30f, 0.76f);

    const QVector3D horizonColor =
        mix(overcastHorizon, clearHorizon, clarity);
    const QVector3D zenithColor =
        mix(overcastZenith, clearZenith, clarity);

    QVector3D rgb =
        mix(
            horizonColor,
            zenithColor,
            altitudeFactor);

    if (m_parameters.showSunGlow &&
        m_parameters.sunDirection.z() > 0.0f) {

        const double sunAngle =
            std::acos(
                clamp(sunCosine, -1.0, 1.0));

        // Display-only circumsolar whitening.
        const double glowWidth =
            (5.0 + 16.0 * (1.0 - clarity))
            * kDegToRad;

        const double glow =
            std::exp(
                -sunAngle
                / std::max(1.0e-6, glowWidth))
            * directStrength
            * (0.25 + 0.75 * clarity);

        const QVector3D warmWhite(
            1.0f, 0.92f, 0.72f);

        rgb = mix(
            rgb,
            warmWhite,
            clamp(glow * 0.65, 0.0, 0.75));
    }

    // The CIE model controls luminance. The RGB hue above is only
    // a natural-looking preview and is not a spectral simulation.
    const double brightness =
        clamp(
            normalizedBrightness * 1.20,
            0.0,
            1.0);

    rgb *= static_cast<float>(brightness);

    return QColor::fromRgbF(
        clamp(rgb.x(), 0.0, 1.0),
        clamp(rgb.y(), 0.0, 1.0),
        clamp(rgb.z(), 0.0, 1.0));
}

QImage SkyPerspectiveWidget::renderToImage(
    const QSize& imageSize) const
{
    const int width =
        std::max(1, imageSize.width());
    const int height =
        std::max(1, imageSize.height());

    QImage image(
        width,
        height,
        QImage::Format_ARGB32);

    image.fill(QColor(28, 30, 34));

    const double scale = absoluteScale();

    QVector<double> samples(
        width * height,
        0.0);

    double skyPeak = 0.0;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const QVector3D direction =
                cameraRay(
                    x, y, width, height);

            if (direction.z() <= 0.0f)
                continue;

            const double value =
                relativeSkyValue(direction) * scale;

            samples[y * width + x] = value;
            skyPeak = std::max(skyPeak, value);
        }
    }

    const double referenceValue =
        m_parameters.toneMapMode ==
            SkyToneMapMode::AutoPeak
        ? std::max(1.0e-9, skyPeak)
        : std::max(
            1.0e-9,
            m_parameters.displayReferenceValue);

    const QVector3D sun =
        m_parameters.sunDirection.normalized();

    const double sunRadiusRadians =
        m_parameters.sunAngularRadiusDeg
        * kDegToRad;

    const double cosSunRadius =
        std::cos(sunRadiusRadians);

    const double sunSolidAngle =
        2.0 * kPi
        * (1.0 - std::cos(sunRadiusRadians));

    const double directDiskValue =
        sunSolidAngle > 1.0e-12
        ? m_parameters.directNormalValue
            / sunSolidAngle
        : 0.0;

    const double directStrength =
        clamp(
            m_parameters.directNormalValue / 800.0,
            0.0,
            1.0);

    for (int y = 0; y < height; ++y) {
        QRgb* scanline =
            reinterpret_cast<QRgb*>(
                image.scanLine(y));

        for (int x = 0; x < width; ++x) {
            const QVector3D direction =
                cameraRay(
                    x, y, width, height);

            if (direction.z() <= 0.0f) {
                scanline[x] =
                    QColor(35, 37, 40).rgba();
                continue;
            }

            const double value =
                samples[y * width + x];

            const double normalized =
                toneMappedValue(
                    value,
                    referenceValue);

            const double sunCosine =
                QVector3D::dotProduct(
                    direction,
                    sun);

            QColor color;

            switch (m_parameters.colorMode) {
            case SkyColorMode::FalseColor:
                color = falseColor(normalized);
                break;

            case SkyColorMode::NaturalPreview:
                color = naturalPreviewColor(
                    direction,
                    normalized,
                    sunCosine,
                    directStrength);
                break;

            case SkyColorMode::GrayscaleLuminance:
            default:
                color = QColor::fromRgbF(
                    normalized,
                    normalized,
                    normalized);
                break;
            }

            if (m_parameters.showSunDisk &&
                m_parameters.directNormalValue > 0.0 &&
                sun.z() > 0.0f &&
                sunCosine >= cosSunRadius) {

                const double sunNormalized =
                    toneMappedValue(
                        directDiskValue,
                        referenceValue);

                const double warm =
                    clamp(
                        0.75 + 0.25 * sunNormalized,
                        0.0,
                        1.0);

                color = QColor::fromRgbF(
                    warm,
                    warm * 0.97,
                    warm * 0.86);
            }

            scanline[x] = color.rgba();
        }
    }

    return image;
}

bool SkyPerspectiveWidget::savePng(
    const QString& filePath,
    const QSize& imageSize) const
{
    return renderToImage(imageSize)
        .save(filePath, "PNG");
}

void SkyPerspectiveWidget::rebuildPreview()
{
    if (width() <= 0 || height() <= 0)
        return;

    // Keep mouse interaction responsive. Export can use any resolution.
    const QSize previewSize =
        size().boundedTo(QSize(800, 520));

    m_preview =
        renderToImage(previewSize);
}

void SkyPerspectiveWidget::paintEvent(
    QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(
        rect(),
        QColor(28, 30, 34));

    if (m_preview.isNull())
        rebuildPreview();

    if (!m_preview.isNull())
        painter.drawImage(rect(), m_preview);

    painter.setRenderHint(
        QPainter::Antialiasing,
        true);

    if (m_parameters.showHorizon) {
        QPainterPath horizonPath;
        bool started = false;

        const int samples =
            std::max(64, width());

        for (int sample = 0;
             sample < samples;
             ++sample) {

            const int x =
                static_cast<int>(
                    sample
                    * (width() - 1.0)
                    / std::max(1, samples - 1));

            int bestY = -1;
            double bestAbsoluteZ =
                std::numeric_limits<double>::max();

            for (int y = 0;
                 y < height();
                 y += 2) {

                const double absoluteZ =
                    std::abs(
                        cameraRay(
                            x,
                            y,
                            width(),
                            height()).z());

                if (absoluteZ < bestAbsoluteZ) {
                    bestAbsoluteZ = absoluteZ;
                    bestY = y;
                }
            }

            if (bestY >= 0 &&
                bestAbsoluteZ < 0.05) {

                if (!started) {
                    horizonPath.moveTo(x, bestY);
                    started = true;
                } else {
                    horizonPath.lineTo(x, bestY);
                }
            }
        }

        painter.setPen(
            QPen(
                QColor(255, 255, 255, 150),
                1.0));

        painter.drawPath(horizonPath);
    }

    painter.setPen(
        QColor(255, 255, 255, 220));

    painter.drawText(
        12,
        22,
        QString(
            "View Az %1°  View Alt %2°  VFOV %3°")
            .arg(
                m_parameters.cameraAzimuthDeg,
                0,
                'f',
                1)
            .arg(
                m_parameters.cameraPitchDeg,
                0,
                'f',
                1)
            .arg(
                m_parameters.verticalFovDeg,
                0,
                'f',
                1));
}

void SkyPerspectiveWidget::resizeEvent(
    QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    rebuildPreview();
}

void SkyPerspectiveWidget::mousePressEvent(
    QMouseEvent* event)
{
    m_lastMousePosition = event->pos();
    event->accept();
}

void SkyPerspectiveWidget::mouseMoveEvent(
    QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;

    const QPoint delta =
        event->pos() - m_lastMousePosition;

    m_lastMousePosition =
        event->pos();

    m_parameters.cameraAzimuthDeg -=
        delta.x() * 0.25;

    while (
        m_parameters.cameraAzimuthDeg < 0.0)
        m_parameters.cameraAzimuthDeg += 360.0;

    while (
        m_parameters.cameraAzimuthDeg >= 360.0)
        m_parameters.cameraAzimuthDeg -= 360.0;

    m_parameters.cameraPitchDeg =
        clamp(
            m_parameters.cameraPitchDeg
                + delta.y() * 0.20,
            -89.9,
            89.9);

    rebuildPreview();
    update();

    emit cameraChanged(
        m_parameters.cameraAzimuthDeg,
        m_parameters.cameraPitchDeg,
        m_parameters.verticalFovDeg);
}

void SkyPerspectiveWidget::wheelEvent(
    QWheelEvent* event)
{
    const double steps =
        event->angleDelta().y() / 120.0;

    m_parameters.verticalFovDeg =
        clamp(
            m_parameters.verticalFovDeg
                - steps * 5.0,
            0,
            179.9);

    rebuildPreview();
    update();

    emit cameraChanged(
        m_parameters.cameraAzimuthDeg,
        m_parameters.cameraPitchDeg,
        m_parameters.verticalFovDeg);

    event->accept();
}
