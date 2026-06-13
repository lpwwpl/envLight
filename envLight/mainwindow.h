#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
//#include "vtk_scene.h"
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

private:
    void setupUI();
    void setupConnections();


    VTKSceneWidget* m_vtkWidget;
    PanoramaLabel* m_panoramaLabel;
    QLabel* m_perspectiveLabel;

    // 参数控件
    QDoubleSpinBox* m_cxSpin, * m_cySpin, * m_czSpin;
    QDoubleSpinBox* m_yawSpin, * m_pitchSpin, * m_rollSpin;
    QDoubleSpinBox* m_hfovSpin, * m_vfovSpin;
    QSpinBox* m_outWSpin, * m_outHSpin;
    QPushButton* m_loadBtn;
    QPushButton* m_updateBtn;

    Image m_panorama;
    bool m_hasPanorama;
};

#endif