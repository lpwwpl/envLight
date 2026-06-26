#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QPixmap>
#include "vtk_scene.h"
#include "environment_light.h"
#include <memory>
#include <cmath>

#define M_PI 3.14159265358979323846

// ================================================================
// 生成环境光全景图（带 Reinhard 色调映射和曝光控制）
// ================================================================
Image generateEnvironmentPanorama(const EnvironmentLight& model,
    int width, int height,
    double exposure) {
    Image img(width, height);
    for (int y = 0; y < height; ++y) {
        double v = double(y) / (height - 1);
        double theta = M_PI * v;
        for (int x = 0; x < width; ++x) {
            double u = double(x) / (width - 1);
            double phi = 2.0 * M_PI * u;
            Direction dir{ theta, phi };
            auto sample = model.sample(dir);
            // 应用曝光
            double r = sample.color.r * sample.radiance * exposure;
            double g = sample.color.g * sample.radiance * exposure;
            double b = sample.color.b * sample.radiance * exposure;
            // Reinhard 色调映射
            double lum = 0.2126 * r + 0.7152 * g + 0.0722 * b;
            double scale = 1.0 / (1.0 + lum);
            r *= scale; g *= scale; b *= scale;
            // 钳位
            r = std::max(0.0, std::min(1.0, r));
            g = std::max(0.0, std::min(1.0, g));
            b = std::max(0.0, std::min(1.0, b));
            img.at(x, y) = sRGB(uint8_t(r * 255), uint8_t(g * 255), uint8_t(b * 255));
        }
    }
    return img;
}

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

    // ---------- 左侧面板 ----------
    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setAlignment(Qt::AlignTop);

    // 相机参数分组
    QGroupBox* cameraGroup = new QGroupBox("相机参数", this);
    QFormLayout* formLayout = new QFormLayout(cameraGroup);

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
    formLayout->addRow(m_loadBtn);

    cameraGroup->setLayout(formLayout);
    leftLayout->addWidget(cameraGroup);

    // ---------- 环境光源生成分组 ----------
    QGroupBox* envGroup = new QGroupBox("环境光源生成", this);
    QFormLayout* envLayout = new QFormLayout(envGroup);

    m_modelCombo = new QComboBox(this);
    m_modelCombo->addItem("CIE 全阴天 (Overcast)");
    m_modelCombo->addItem("CIE 一般天空 (General)");
    m_modelCombo->addItem("U.S. Standard Atmosphere 1976");
    envLayout->addRow("模型:", m_modelCombo);

    m_zenithLuminanceSpin = new QDoubleSpinBox(this);
    m_zenithLuminanceSpin->setRange(0.1, 10000.0);
    m_zenithLuminanceSpin->setValue(100.0);
    m_zenithLuminanceSpin->setSingleStep(10.0);
    envLayout->addRow("天顶辐射度 Lz:", m_zenithLuminanceSpin);

    m_sunThetaSpin = new QDoubleSpinBox(this);
    m_sunThetaSpin->setRange(0, 90);
    m_sunThetaSpin->setValue(30);
    m_sunThetaSpin->setSuffix("°");
    envLayout->addRow("太阳天顶角:", m_sunThetaSpin);

    m_sunPhiSpin = new QDoubleSpinBox(this);
    m_sunPhiSpin->setRange(-180, 180);
    m_sunPhiSpin->setValue(0);
    m_sunPhiSpin->setSuffix("°");
    envLayout->addRow("太阳方位角:", m_sunPhiSpin);

    m_skyTypeSpin = new QSpinBox(this);
    m_skyTypeSpin->setRange(1, 15);
    m_skyTypeSpin->setValue(12);
    envLayout->addRow("天空类型 (1-15):", m_skyTypeSpin);

    m_altitudeSpin = new QDoubleSpinBox(this);
    m_altitudeSpin->setRange(0, 100);
    m_altitudeSpin->setValue(0);
    m_altitudeSpin->setSuffix(" km");
    envLayout->addRow("海拔:", m_altitudeSpin);

    // 曝光控制
    m_exposureSpin = new QDoubleSpinBox(this);
    m_exposureSpin->setRange(0.01, 10.0);
    m_exposureSpin->setValue(0.5);
    m_exposureSpin->setSingleStep(0.05);
    envLayout->addRow("曝光 (Exposure):", m_exposureSpin);

    // 暖色强度控制 (新增)
    m_warmIntensitySpin = new QDoubleSpinBox(this);
    m_warmIntensitySpin->setRange(0.0, 2.0);
    m_warmIntensitySpin->setValue(1.0);
    m_warmIntensitySpin->setSingleStep(0.05);
    envLayout->addRow("暖色强度:", m_warmIntensitySpin);

    m_generateEnvBtn = new QPushButton("生成环境光全景图", this);
    envLayout->addRow(m_generateEnvBtn);

    envGroup->setLayout(envLayout);
    leftLayout->addWidget(envGroup);

    leftLayout->addStretch();
    mainLayout->addWidget(leftPanel, 1);

    // 环境光预览（放在左侧底部）
    m_envPreviewLabel = new QLabel(this);
    m_envPreviewLabel->setAlignment(Qt::AlignCenter);
    m_envPreviewLabel->setFixedSize(400, 200);
    m_envPreviewLabel->setStyleSheet("border: 1px solid gray; background-color: #2a2a2a;");
    m_envPreviewLabel->setText("未生成环境光");
    leftLayout->addWidget(m_envPreviewLabel);

    // ---------- 右侧显示区域 ----------
    QVBoxLayout* rightLayout = new QVBoxLayout;

    m_vtkWidget = new VTKSceneWidget(this);
    m_vtkWidget->setFixedSize(600, 300);
    rightLayout->addWidget(m_vtkWidget);

    m_panoramaLabel = new PanoramaLabel(this);
    m_panoramaLabel->setAlignment(Qt::AlignCenter);
    m_panoramaLabel->setFixedSize(600, 300);
    m_panoramaLabel->setText("未加载全景图");
    rightLayout->addWidget(m_panoramaLabel);

    m_perspectiveLabel = new QLabel(this);
    m_perspectiveLabel->setAlignment(Qt::AlignCenter);
    m_perspectiveLabel->setFixedSize(600, 300);
    m_perspectiveLabel->setStyleSheet("border: 1px solid gray; background-color: #2a2a2a;");
    m_perspectiveLabel->setText("透视视图");
    rightLayout->addWidget(m_perspectiveLabel);

    mainLayout->addLayout(rightLayout, 2);

    connect(m_vtkWidget, &VTKSceneWidget::perspectiveViewReady,
        this, &MainWindow::onPerspectiveViewReady);

    onModelChanged(0);
}

void MainWindow::setupConnections() {
    connect(m_loadBtn, &QPushButton::clicked, this, &MainWindow::onLoadImage);
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

    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::onModelChanged);
    connect(m_generateEnvBtn, &QPushButton::clicked,
        this, &MainWindow::onGenerateEnvironment);
}

void MainWindow::onModelChanged(int index) {
    bool isGeneral = (index == 1);
    bool isUSStd = (index == 2);
    m_sunThetaSpin->setVisible(isGeneral || isUSStd);
    m_sunPhiSpin->setVisible(isGeneral);
    m_skyTypeSpin->setVisible(isGeneral);
    m_altitudeSpin->setVisible(isUSStd);
}

void MainWindow::onLoadImage() {
    QString fileName = QFileDialog::getOpenFileName(this, "打开全景图", "",
        "图像文件 (*.jpg *.jpeg *.png *.bmp *.tga *.exr);;所有文件 (*.*)");
    if (fileName.isEmpty()) return;
    Image img;
    float exposure = 1.0f, gamma = 2.2f;
    if (!PanoramaProcessor::loadImage(fileName.toStdString(), img, exposure, gamma)) {
        QMessageBox::warning(this, "错误", "加载图像失败！");
        return;
    }
    m_panorama = img;
    m_hasPanorama = true;
    m_vtkWidget->setPanorama(img);
    m_vtkWidget->update();
    m_panoramaLabel->setPanoramaImage(img);

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

// ---------- 环境光生成 ----------
void MainWindow::onGenerateEnvironment() {
    int modelIdx = m_modelCombo->currentIndex();
    double Lz = m_zenithLuminanceSpin->value();
    double sunTheta = m_sunThetaSpin->value() * M_PI / 180.0;
    double sunPhi = m_sunPhiSpin->value() * M_PI / 180.0;
    int skyType = m_skyTypeSpin->value();
    double altitude = m_altitudeSpin->value();
    double exposure = m_exposureSpin->value();
    double warmIntensity = m_warmIntensitySpin->value();

    std::unique_ptr<EnvironmentLight> model;
    switch (modelIdx) {
    case 0: // 全阴天
        model = std::make_unique<CIEOvercastSky>(Lz);
        break;
    case 1: // 一般天空 (使用新的 CIESkyModel)
    {
        CieSkyType type = static_cast<CieSkyType>(std::max(1, std::min(skyType, 15)));
        model = std::make_unique<CIESkyModel>(type, sunTheta, sunPhi, Lz, sRGB(1.0, 1.0, 0.9), warmIntensity);
        break;
    }
    case 2: // US 大气
        model = std::make_unique<USStandardAtmosphere1976Light>(sunTheta, altitude, 1.0);
        break;
    default:
        return;
    }

    m_generateEnvBtn->setEnabled(false);
    m_envPreviewLabel->setText("生成中...");

    const int W = 1024, H = 512;
    Image img = generateEnvironmentPanorama(*model, W, H, exposure);
    QImage qimg(img.width, img.height, QImage::Format_RGB888);
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            const sRGB& c = img.at(x, y);
            qimg.setPixel(x, y, qRgb(c.r, c.g, c.b));
        }
    }

    if (!qimg.isNull()) {
        QPixmap pix = QPixmap::fromImage(qimg);
        pix = pix.scaled(m_envPreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_envPreviewLabel->setPixmap(pix);
    }
    else {
        m_envPreviewLabel->setText("生成失败");
    }

    m_generateEnvBtn->setEnabled(true);
}