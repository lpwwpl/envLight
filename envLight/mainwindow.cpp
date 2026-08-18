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
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSignalBlocker>

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
    : QMainWindow(parent), m_hasPanorama(false), m_coordinateSystem(WorldCoordinateSystem::ENU) {
    setWindowTitle("Main Application");
    createMenu();
    setupUI();
    setupConnections();
}

MainWindow::~MainWindow() {}

void MainWindow::showSkyViewer() {
    // 每次创建新的 MainWidget 窗口并显示
    CIEWidget* viewer = new CIEWidget();
    viewer->setAttribute(Qt::WA_DeleteOnClose); // 关闭时自动删除
    viewer->show();
}

void MainWindow::createMenu() {
    // 创建菜单栏
    m_menuBar = this->menuBar();

    // 添加 "View" 菜单
    QMenu* viewMenu = m_menuBar->addMenu("&CIE Sky");

    // 添加一个 Action
    QAction* skyAction = new QAction("CIE Sky Viewer", this);
    viewMenu->addAction(skyAction);

    // 连接信号
    connect(skyAction, &QAction::triggered, this, &MainWindow::showSkyViewer);
}

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

    m_coordinateSystemCombo = new QComboBox;
    m_coordinateSystemCombo->addItem("ENU - East / North / Up", static_cast<int>(WorldCoordinateSystem::ENU));
    m_coordinateSystemCombo->addItem("NED - North / East / Down", static_cast<int>(WorldCoordinateSystem::NED));
    m_coordinateSystemCombo->setCurrentIndex(0); // Default: ENU
    m_coordinateSystemCombo->setToolTip(
        "ENU: X=East, Y=North, Z=Up（默认）\n"
        "NED: X=North, Y=East, Z=Down\n"
        "切换坐标系时相机位置会自动换算，方位角/仰角保持不变。");
    formLayout->addRow("世界坐标系:", m_coordinateSystemCombo);

    m_cxSpin = new QDoubleSpinBox; m_cxSpin->setRange(-1, 1); m_cxSpin->setSingleStep(0.05); m_cxSpin->setValue(0.5);
    m_cySpin = new QDoubleSpinBox; m_cySpin->setRange(-1, 1); m_cySpin->setSingleStep(0.05); m_cySpin->setValue(0.2);
    m_czSpin = new QDoubleSpinBox; m_czSpin->setRange(-1, 1); m_czSpin->setSingleStep(0.05); m_czSpin->setValue(0.3);
    formLayout->addRow("相机 X:", m_cxSpin);
    formLayout->addRow("相机 Y:", m_cySpin);
    formLayout->addRow("相机 Z:", m_czSpin);
    m_cxSpin->setToolTip("ENU 默认：X=East；NED 模式：X=North");
    m_cySpin->setToolTip("ENU 默认：Y=North；NED 模式：Y=East");
    m_czSpin->setToolTip("ENU 默认：Z=Up；NED 模式：Z=Down");

    m_yawSpin = new QDoubleSpinBox;
    m_yawSpin->setRange(0.0, 360.0);
    m_yawSpin->setDecimals(1);
    m_yawSpin->setSingleStep(1.0);
    m_yawSpin->setValue(30.0);
    m_yawSpin->setSuffix("°");
    m_yawSpin->setWrapping(true);
    m_yawSpin->setToolTip("方位角：0°=北(N)，90°=东(E)，180°=南(S)，270°=西(W)。");

    m_pitchSpin = new QDoubleSpinBox;
    m_pitchSpin->setRange(-89.9, 89.9);
    m_pitchSpin->setDecimals(1);
    m_pitchSpin->setSingleStep(1.0);
    m_pitchSpin->setValue(20.0);
    m_pitchSpin->setSuffix("°");
    m_pitchSpin->setToolTip("仰角：正值向上，负值向下；0°=水平。ENU/NED 下物理含义保持一致。");

    m_rollSpin = new QDoubleSpinBox;
    m_rollSpin->setRange(-180.0, 180.0);
    m_rollSpin->setDecimals(1);
    m_rollSpin->setSingleStep(1.0);
    m_rollSpin->setValue(0.0);
    m_rollSpin->setSuffix("°");

    formLayout->addRow("方位角 (Yaw/Azimuth):", m_yawSpin);
    formLayout->addRow("仰角 (Elevation):", m_pitchSpin);
    formLayout->addRow("横滚角 (Roll):", m_rollSpin);

    m_hfovSpin = new QDoubleSpinBox; m_hfovSpin->setRange(10, 170); m_hfovSpin->setValue(90);
    m_vfovSpin = new QDoubleSpinBox; m_vfovSpin->setRange(10, 170); m_vfovSpin->setValue(60);
    formLayout->addRow("水平 FOV:", m_hfovSpin);
    formLayout->addRow("垂直 FOV:", m_vfovSpin);

    m_outWSpin = new QSpinBox; m_outWSpin->setRange(64, 2048); m_outWSpin->setValue(800);
    m_outHSpin = new QSpinBox; m_outHSpin->setRange(64, 2048); m_outHSpin->setValue(600);
    formLayout->addRow("输出宽度:", m_outWSpin);
    formLayout->addRow("输出高度:", m_outHSpin);

    // 全景方向校准：指定源等距柱状图中“北”所在的水平位置。
    // 0°=最左接缝，180°=图像中心；E/S/W 自动每隔 90° 推导。
    m_northDirectionSpin = new QDoubleSpinBox;
    m_northDirectionSpin->setRange(0.0, 360.0);
    m_northDirectionSpin->setDecimals(1);
    m_northDirectionSpin->setSingleStep(1.0);
    m_northDirectionSpin->setValue(180.0);
    m_northDirectionSpin->setSuffix("°");
    m_northDirectionSpin->setWrapping(true);
    m_northDirectionSpin->setToolTip(
        "设置源全景图中北向(N)的水平位置：0°=左边接缝，90°=1/4处，180°=中心。\n"
        "东(E)、南(S)、西(W)将自动位于 N+90°、N+180°、N+270°。");
    formLayout->addRow("全景北向位置:", m_northDirectionSpin);

    m_loadBtn = new QPushButton("加载全景图");
    formLayout->addRow(m_loadBtn);

    cameraGroup->setLayout(formLayout);
    leftLayout->addWidget(cameraGroup);

    leftLayout->addStretch();
    mainLayout->addWidget(leftPanel, 1);

    // ---------- 右侧显示区域 ----------
    QVBoxLayout* rightLayout = new QVBoxLayout;

    m_vtkWidget = new VTKSceneWidget(this);
    m_vtkWidget->setCoordinateSystem(m_coordinateSystem);
    m_vtkWidget->setNorthDirectionDegrees(m_northDirectionSpin->value());
    m_vtkWidget->setFixedSize(600, 300);
    rightLayout->addWidget(m_vtkWidget);

    m_panoramaLabel = new PanoramaLabel(this);
    m_panoramaLabel->setNorthDirectionDegrees(m_northDirectionSpin->value());
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

    //onModelChanged(0);
}

void MainWindow::setupConnections() {
    connect(m_loadBtn, &QPushButton::clicked, this, &MainWindow::onLoadImage);
    connect(m_coordinateSystemCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::onCoordinateSystemChanged);
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
    connect(m_northDirectionSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &MainWindow::onNorthDirectionChanged);

    //connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
    //    this, &MainWindow::onModelChanged);
    //connect(m_generateEnvBtn, &QPushButton::clicked,
    //    this, &MainWindow::onGenerateEnvironment);
}

void MainWindow::onLoadImage() {
    QString fileName = QFileDialog::getOpenFileName(this, "打开全景图", "",
        "图像文件 (*.jpg *.jpeg *.png *.bmp *.tga *.exr *.hdr);;所有文件 (*.*)");
    if (fileName.isEmpty()) return;

    QByteArray pathUtf8 = fileName.toUtf8();
    HDRImage img;
    if (!PanoramaProcessor::loadImage(std::string(pathUtf8.constData()), img)) {
        QMessageBox::warning(this, "错误", "加载图像失败！");
        return;
    }

    // Keep the original scene-linear HDR image for all calculations/perspective sampling.
    m_panorama = img;
    m_hasPanorama = true;
    m_vtkWidget->setCoordinateSystem(m_coordinateSystem);
    m_vtkWidget->setNorthDirectionDegrees(m_northDirectionSpin->value());
    m_panoramaLabel->setNorthDirectionDegrees(m_northDirectionSpin->value());
    m_vtkWidget->setPanorama(m_panorama);
    m_vtkWidget->update();

    // Tone mapping is display-only. It does not change m_panorama.
    Image preview = PanoramaProcessor::toneMapForDisplay(m_panorama, 1.0f, 2.2f);
    m_panoramaLabel->setPanoramaImage(preview);

    onUpdateParameters();
    QMessageBox::information(this, "成功",
        QString("全景图加载成功: %1x%2（HDR/EXR 原始浮点亮度已保留）")
        .arg(img.width).arg(img.height));
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
    std::vector<QPointF> corners = p.computeCornerUVs(
        cx, cy, cz, yaw, pitch, roll, hfov, vfov, outW, outH,
        m_northDirectionSpin->value(), m_coordinateSystem);
    if (corners.size() >= 3) {
        m_panoramaLabel->setCorners(corners);
    }
    else {
        m_panoramaLabel->clearCorners();
    }

    m_vtkWidget->setCameraParameters(cx, cy, cz, yaw, pitch, roll, hfov, vfov, outW, outH);
}

void MainWindow::onCoordinateSystemChanged(int index) {
    WorldCoordinateSystem newSystem = static_cast<WorldCoordinateSystem>(
        m_coordinateSystemCombo->itemData(index).toInt());
    if (newSystem == m_coordinateSystem) return;

    // Preserve the same physical camera location while changing only its coordinate representation.
    const double oldPos[3] = { m_cxSpin->value(), m_cySpin->value(), m_czSpin->value() };
    double newPos[3];
    PanoramaProcessor::convertCoordinateVector(oldPos, m_coordinateSystem, newSystem, newPos);

    {
        QSignalBlocker bx(m_cxSpin);
        QSignalBlocker by(m_cySpin);
        QSignalBlocker bz(m_czSpin);
        m_cxSpin->setValue(newPos[0]);
        m_cySpin->setValue(newPos[1]);
        m_czSpin->setValue(newPos[2]);
    }

    m_coordinateSystem = newSystem;
    m_vtkWidget->setCoordinateSystem(m_coordinateSystem);

    if (m_coordinateSystem == WorldCoordinateSystem::ENU) {
        m_cxSpin->setToolTip("X = East");
        m_cySpin->setToolTip("Y = North");
        m_czSpin->setToolTip("Z = Up");
    } else {
        m_cxSpin->setToolTip("X = North");
        m_cySpin->setToolTip("Y = East");
        m_czSpin->setToolTip("Z = Down");
    }

    if (m_hasPanorama) onUpdateParameters();
}

void MainWindow::onNorthDirectionChanged(double value) {
    // The source panorama itself is not resampled/rotated here. We only change the
    // mapping between geographic world directions and source panorama U.
    m_panoramaLabel->setNorthDirectionDegrees(value);
    m_vtkWidget->setNorthDirectionDegrees(value);

    if (m_hasPanorama) {
        onUpdateParameters();
    }
}

void MainWindow::onPerspectiveViewReady(const QImage& img) {
    QPixmap pix = QPixmap::fromImage(img);
    pix = pix.scaled(m_perspectiveLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_perspectiveLabel->setPixmap(pix);
}

