#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QVTKOpenGLWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <QTimer>
#include <QImage>
#include <QPixmap>
#include "vtk_scene.h"
#include <vtkRenderer.h>

#define M_PI 3.1415926
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_hasPanorama(false) {
    setupUI();
    setupConnections();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    QHBoxLayout* mainLayout = new QHBoxLayout(central);

    // 左侧控制面板
    QGroupBox* controlGroup = new QGroupBox("参数设置");
    QFormLayout* formLayout = new QFormLayout(controlGroup);

    m_cxSpin = new QDoubleSpinBox; m_cxSpin->setRange(-1, 1); m_cxSpin->setSingleStep(0.05); m_cxSpin->setValue(0.5);
    m_cySpin = new QDoubleSpinBox; m_cySpin->setRange(-1, 1); m_cySpin->setSingleStep(0.05); m_cySpin->setValue(0.2);
    m_czSpin = new QDoubleSpinBox; m_czSpin->setRange(-1, 1); m_czSpin->setSingleStep(0.05); m_czSpin->setValue(0.3);
    formLayout->addRow("相机 X:", m_cxSpin);
    formLayout->addRow("相机 Y:", m_cySpin);
    formLayout->addRow("相机 Z:", m_czSpin);

    m_yawSpin = new QDoubleSpinBox; m_yawSpin->setRange(-180, 180); m_yawSpin->setValue(30);
    m_pitchSpin = new QDoubleSpinBox; m_pitchSpin->setRange(-180, 180); m_pitchSpin->setValue(20);
    m_rollSpin = new QDoubleSpinBox; m_rollSpin->setRange(-180, 180); m_rollSpin->setValue(0);
    formLayout->addRow("偏航 (Yaw):", m_yawSpin);
    formLayout->addRow("俯仰 (Pitch):", m_pitchSpin);
    formLayout->addRow("横滚 (Roll):", m_rollSpin);

    m_hfovSpin = new QDoubleSpinBox; m_hfovSpin->setRange(10, 170); m_hfovSpin->setValue(90);
    m_vfovSpin = new QDoubleSpinBox; m_vfovSpin->setRange(10, 170); m_vfovSpin->setValue(60);
    formLayout->addRow("水平 FOV:", m_hfovSpin);
    formLayout->addRow("垂直 FOV:", m_vfovSpin);

    m_outWSpin = new QSpinBox; m_outWSpin->setRange(64, 2048); m_outWSpin->setValue(800);
    m_outHSpin = new QSpinBox; m_outHSpin->setRange(64, 2048); m_outHSpin->setValue(600);
    formLayout->addRow("输出宽度:", m_outWSpin);
    formLayout->addRow("输出高度:", m_outHSpin);

    m_loadBtn = new QPushButton("加载全景图");
    m_updateBtn = new QPushButton("更新参数");
    formLayout->addRow(m_loadBtn);
    formLayout->addRow(m_updateBtn);

    controlGroup->setLayout(formLayout);
    mainLayout->addWidget(controlGroup, 1);

    // 右侧 VTK 和透视视图显示
    QVBoxLayout* rightLayout = new QVBoxLayout;
    m_vtkWidget = new VTKSceneWidget(this);
    m_vtkWidget->setFixedSize(600, 300);
    rightLayout->addWidget(m_vtkWidget);

    m_panoramaLabel = new PanoramaLabel(this);
    m_panoramaLabel->setAlignment(Qt::AlignCenter);
    m_panoramaLabel->setFixedSize(600, 300);
    rightLayout->addWidget(m_panoramaLabel);

    m_perspectiveLabel = new QLabel(this);
    m_perspectiveLabel->setAlignment(Qt::AlignCenter);
    m_perspectiveLabel->setFixedSize(600, 300);
    m_perspectiveLabel->setStyleSheet("border: 1px solid gray; background-color: #2a2a2a;");
    rightLayout->addWidget(m_perspectiveLabel);
    mainLayout->addLayout(rightLayout, 2);


    //m_vtkScene->setRenderWindow(m_vtkWidget->renderWindow());
    connect(m_vtkWidget, &VTKSceneWidget::perspectiveViewReady, this, &MainWindow::onPerspectiveViewReady);
}

void MainWindow::setupConnections() {
    connect(m_loadBtn, &QPushButton::clicked, this, &MainWindow::onLoadImage);
    connect(m_updateBtn, &QPushButton::clicked, this, &MainWindow::onUpdateParameters);
    connect(m_cxSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onUpdateParameters);
    connect(m_cySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onUpdateParameters);
    connect(m_czSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onUpdateParameters);
    connect(m_yawSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onUpdateParameters);
    connect(m_pitchSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onUpdateParameters);
    connect(m_rollSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onUpdateParameters);
    connect(m_hfovSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onUpdateParameters);
    connect(m_vfovSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onUpdateParameters);
    connect(m_outWSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onUpdateParameters);
    connect(m_outHSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onUpdateParameters);
}

void MainWindow::onLoadImage() {
    QString fileName = QFileDialog::getOpenFileName(this, "打开全景图", "",
        "图像文件 (*.jpg *.jpeg *.png *.bmp *.tga *.exr);;所有文件 (*.*)");
    if (fileName.isEmpty()) return;
    Image img;
    float exposure = 1.0f, gamma = 2.2f; // 从控件获取可选的
    if (!PanoramaProcessor::loadImage(fileName.toStdString(), img, exposure, gamma)) {
        QMessageBox::warning(this, "错误", "加载图像失败！");
        return;
    }
    m_panorama = img;
    m_hasPanorama = true;
    // 设置纹理到 VTK 球体
    m_vtkWidget->setPanorama(img);
    m_vtkWidget->update();
    m_panoramaLabel->setPanoramaImage(img);

    // 立即更新一次参数
    onUpdateParameters();
    QMessageBox::information(this, "成功", QString("全景图加载成功: %1x%2").arg(img.width).arg(img.height));
}


void MainWindow::onUpdateParameters() {
    if (!m_hasPanorama) return;
    double cx = m_cxSpin->value();
    double cy = m_cySpin->value();
    double cz = m_czSpin->value();
    double yaw = m_yawSpin->value();
    double pitch = m_pitchSpin->value();
    double roll = m_rollSpin->value();
    double hfov = m_hfovSpin->value();
    double vfov = m_vfovSpin->value();
    int outW = m_outWSpin->value();
    int outH = m_outHSpin->value();


    PanoramaProcessor p;
    std::vector<QPointF> corners = p.computeCornerUVs(cx, cy, cz, yaw, pitch, roll, hfov, vfov, outW, outH);
    if (corners.size() >= 3) {
        m_panoramaLabel->setCorners(corners);
    }
    else {
        m_panoramaLabel->clearCorners();
    }

    m_vtkWidget->setCameraParameters(cx, cy, cz, yaw, pitch, roll, hfov, vfov, outW, outH);
}

void MainWindow::onPerspectiveViewReady(const QImage& img) {
    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(m_perspectiveLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_perspectiveLabel->setPixmap(pix);
}