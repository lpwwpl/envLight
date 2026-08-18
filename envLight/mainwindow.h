#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include "CIEWidget.h"
#include "panorama_processor.h"
class QMenuBar;

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
    void createMenu();
    void showSkyViewer();
private:
    void setupUI();
    void setupConnections();

    VTKSceneWidget* m_vtkWidget;
    PanoramaLabel* m_panoramaLabel;

    // 原有相机参数
    QDoubleSpinBox* m_cxSpin, * m_cySpin, * m_czSpin;
    QDoubleSpinBox* m_yawSpin, * m_pitchSpin, * m_rollSpin;
    QDoubleSpinBox* m_hfovSpin, * m_vfovSpin;
    QSpinBox* m_outWSpin, * m_outHSpin;
    QPushButton* m_loadBtn;
    QLabel* m_perspectiveLabel;

    CIEWidget* m_cieWidget;
    QMenuBar* m_menuBar;

    HDRImage m_panorama;
    bool m_hasPanorama;
};

#endif // MAINWINDOW_H