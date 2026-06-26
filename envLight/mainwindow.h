#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

#include "panorama_processor.h"

class PanoramaLabel;
class VTKSceneWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onLoadImage();
    void onUpdateParameters();
    void onPerspectiveViewReady(const QImage& img);
    void onGenerateEnvironment();
    void onModelChanged(int index);

private:
    void setupUI();
    void setupConnections();

    VTKSceneWidget* m_vtkWidget;
    PanoramaLabel* m_panoramaLabel;

    // 环境光参数控件
    QComboBox* m_modelCombo;
    QDoubleSpinBox* m_zenithLuminanceSpin;  // Lz
    QDoubleSpinBox* m_sunThetaSpin;         // 太阳天顶角
    QDoubleSpinBox* m_sunPhiSpin;           // 太阳方位角
    QSpinBox* m_skyTypeSpin;                // 天空类型 (1-15)
    QDoubleSpinBox* m_altitudeSpin;         // 海拔 (km)
    QDoubleSpinBox* m_exposureSpin;         // 曝光控制
    QDoubleSpinBox* m_warmIntensitySpin;    // 暖色强度 (新增)
    QPushButton* m_generateEnvBtn;
    QLabel* m_envPreviewLabel;

    // 原有相机参数
    QDoubleSpinBox* m_cxSpin, * m_cySpin, * m_czSpin;
    QDoubleSpinBox* m_yawSpin, * m_pitchSpin, * m_rollSpin;
    QDoubleSpinBox* m_hfovSpin, * m_vfovSpin;
    QSpinBox* m_outWSpin, * m_outHSpin;
    QPushButton* m_loadBtn;
    QLabel* m_perspectiveLabel;

    Image m_panorama;
    bool m_hasPanorama;
};

#endif // MAINWINDOW_H