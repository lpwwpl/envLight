#ifndef CIEWIDGET_H
#define CIEWIDGET_H

#include <QMainWindow>
#include <QTimer>
#include "EpwData.hpp"

QT_BEGIN_NAMESPACE
class QComboBox;
class QDoubleSpinBox;
class QPushButton;
class QSlider;
class QLabel;
class QDateEdit;
class QTimeEdit;
class QGroupBox;
QT_END_NAMESPACE

class SkyPolarWidget;

class CIEWidget : public QMainWindow {
    Q_OBJECT
public:
    explicit CIEWidget(QWidget* parent = nullptr);
    ~CIEWidget();

private slots:
    void onSkyTypeChanged(int index);
    void onCoefficientChanged();
    void onLoadEPW();
    void onSliderTime(int value);
    void onRenderTimeout();

private:
    void setupUI();
    void applyEpwRecord(const EpwDocument& doc, const EpwRecord& rec);
    double epwMidpointHour(int hour, int minute, int recordsPerHour) const;

    SkyPolarWidget* m_skyWidget = nullptr;

    QComboBox* m_skyTypeCombo = nullptr;
    QDoubleSpinBox* m_spinA = nullptr;
    QDoubleSpinBox* m_spinB = nullptr;
    QDoubleSpinBox* m_spinC = nullptr;
    QDoubleSpinBox* m_spinD = nullptr;
    QDoubleSpinBox* m_spinE = nullptr;

    QPushButton* m_loadEpwBtn = nullptr;
    QSlider* m_timeSlider = nullptr;
    QLabel* m_sliderInfo = nullptr;

    QTimer* m_renderTimer = nullptr;

    EpwDocument m_epwDoc;
    bool m_epwLoaded = false;
};

#endif // MAINWINDOW_H