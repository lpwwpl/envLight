#include "CIEWidget.h"
#include "SkyPolarWidget.h"
#include "EpwReader.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QDateEdit>
#include <QTimeEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

CIEWidget::CIEWidget(QWidget* parent) : QMainWindow(parent)
{
    setupUI();
    m_renderTimer = new QTimer(this);
    m_renderTimer->setSingleShot(true);
    m_renderTimer->setInterval(50);
    connect(m_renderTimer, &QTimer::timeout, this, &CIEWidget::onRenderTimeout);

    // 初始选择天空类型 12 (索引 11)
    m_skyTypeCombo->setCurrentIndex(11);
}

CIEWidget::~CIEWidget() {}

void CIEWidget::setupUI()
{
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    // --- 控件区域 ---
    QHBoxLayout* topLayout = new QHBoxLayout;

    // 天空类型
    QGroupBox* skyGroup = new QGroupBox("CIE Sky Type");
    QVBoxLayout* skyLayout = new QVBoxLayout(skyGroup);
    m_skyTypeCombo = new QComboBox;
    for (int i = 1; i <= 15; ++i) {
        QString name;
        switch (i) {
        case 1: name = "CIE Standard Overcast"; break;
        case 2: name = "Overcast steep grade sun"; break;
        case 3: name = "Overcast moderate no sun"; break;
        case 4: name = "Overcast moderate some sun"; break;
        case 5: name = "CIE Uniform"; break;
        case 6: name = "Partly cloudy no grade sun"; break;
        case 7: name = "Partly cloudy no grade sun 2"; break;
        case 8: name = "Partly cloudy corona"; break;
        case 9: name = "Partly cloudy obscured"; break;
        case 10: name = "Partly cloudy circumscalar"; break;
        case 11: name = "White-blue corona"; break;
        case 12: name = "Clear low turbidity"; break;
        case 13: name = "Clear some pollution"; break;
        case 14: name = "Cloudless turbid corona"; break;
        case 15: name = "White-blue turbid"; break;
        }
        m_skyTypeCombo->addItem(QString("%1. %2").arg(i, 2, 10, QChar('0')).arg(name));
    }
    skyLayout->addWidget(m_skyTypeCombo);
    topLayout->addWidget(skyGroup);

    // ABCDE 系数
    QGroupBox* coeffGroup = new QGroupBox("Perez Coefficients");
    QGridLayout* coeffLayout = new QGridLayout(coeffGroup);
    auto addSpin = [&](const QString& label, QDoubleSpinBox*& spin, double min, double max, double step) {
        QLabel* lbl = new QLabel(label);
        spin = new QDoubleSpinBox;
        spin->setRange(min, max);
        spin->setSingleStep(step);
        spin->setDecimals(2);
        coeffLayout->addWidget(lbl, coeffLayout->rowCount(), 0);
        coeffLayout->addWidget(spin, coeffLayout->rowCount() - 1, 1);
    };
    addSpin("A", m_spinA, -5.0, 5.0, 0.1);
    addSpin("B", m_spinB, -5.0, 5.0, 0.1);
    addSpin("C", m_spinC, -5.0, 10.0, 0.1);
    addSpin("D", m_spinD, -10.0, 5.0, 0.1);
    addSpin("E", m_spinE, -5.0, 5.0, 0.1);
    topLayout->addWidget(coeffGroup);

    mainLayout->addLayout(topLayout);

    // --- EPW 加载及时间滑动 ---
    QHBoxLayout* epwLayout = new QHBoxLayout;
    m_loadEpwBtn = new QPushButton("Load EPW");
    connect(m_loadEpwBtn, &QPushButton::clicked, this, &CIEWidget::onLoadEPW);
    epwLayout->addWidget(m_loadEpwBtn);

    m_timeSlider = new QSlider(Qt::Horizontal);
    m_timeSlider->setRange(0, 0);
    m_timeSlider->setEnabled(false);
    connect(m_timeSlider, &QSlider::sliderMoved, this, &CIEWidget::onSliderTime);
    epwLayout->addWidget(m_timeSlider);

    m_sliderInfo = new QLabel("No EPW");
    epwLayout->addWidget(m_sliderInfo);
    mainLayout->addLayout(epwLayout);

    // --- SkyPolarWidget ---
    m_skyWidget = new SkyPolarWidget;
    mainLayout->addWidget(m_skyWidget);

    // 连接系数信号 (带防抖)
    auto connectCoeff = [this](QDoubleSpinBox* spin) {
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this]() { m_renderTimer->start(); });
    };
    connectCoeff(m_spinA);
    connectCoeff(m_spinB);
    connectCoeff(m_spinC);
    connectCoeff(m_spinD);
    connectCoeff(m_spinE);

    // 天空类型变化直接更新
    connect(m_skyTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &CIEWidget::onSkyTypeChanged);
}

void CIEWidget::onSkyTypeChanged(int index)
{
    // 从标准天空加载系数
    SSLib::CIESkyCoefficients coeffs = SSLib::CIEStandardSkyCoefficients(index);
    // 阻塞信号避免循环
    const QSignalBlocker b1(m_spinA), b2(m_spinB), b3(m_spinC), b4(m_spinD), b5(m_spinE);
    m_spinA->setValue(coeffs.a);
    m_spinB->setValue(coeffs.b);
    m_spinC->setValue(coeffs.c);
    m_spinD->setValue(coeffs.d);
    m_spinE->setValue(coeffs.e);

    // 更新 Widget
    m_skyWidget->setCieSkyType(index);
}

void CIEWidget::onCoefficientChanged()
{
    SSLib::CIESkyCoefficients coeffs;
    coeffs.a = static_cast<float>(m_spinA->value());
    coeffs.b = static_cast<float>(m_spinB->value());
    coeffs.c = static_cast<float>(m_spinC->value());
    coeffs.d = static_cast<float>(m_spinD->value());
    coeffs.e = static_cast<float>(m_spinE->value());
    m_skyWidget->setCustomCoefficients(coeffs);

    // 取消标准类型高亮 (下拉框可选)
    // 不强制修改下拉框，保持用户选择，但内部以自定义模式渲染
}

void CIEWidget::onRenderTimeout()
{
    onCoefficientChanged();
}

void CIEWidget::onLoadEPW()
{
    QString path = QFileDialog::getOpenFileName(this, "Open EPW File", "", "EPW Files (*.epw)");
    if (path.isEmpty()) return;

    EpwDocument doc;
    if (!EpwReader::read(path, doc)) {
        QMessageBox::critical(this, "Error", "Failed to parse EPW file.");
        return;
    }

    m_epwDoc = doc;
    m_epwLoaded = true;

    // 设置地点
    m_skyWidget->setLocation(doc.location.latitude, doc.location.longitude, doc.location.timeZone);

    // 启用滑动条
    if (!doc.records.isEmpty()) {
        m_timeSlider->setRange(0, doc.records.size() - 1);
        m_timeSlider->setEnabled(true);
        // 默认应用第一条记录
        applyEpwRecord(doc, doc.records.first());
        m_timeSlider->setValue(0);
    }
    else {
        m_timeSlider->setRange(0, 0);
        m_timeSlider->setEnabled(false);
        m_sliderInfo->setText("No data");
    }
}

void CIEWidget::onSliderTime(int value)
{
    if (!m_epwLoaded || value < 0 || value >= m_epwDoc.records.size()) return;
    const EpwRecord& rec = m_epwDoc.records[value];
    applyEpwRecord(m_epwDoc, rec);
    m_sliderInfo->setText(QString("%1-%2-%3 %4:%5")
        .arg(rec.year).arg(rec.month, 2, 10, QChar('0'))
        .arg(rec.day, 2, 10, QChar('0'))
        .arg(rec.hour, 2, 10, QChar('0'))
        .arg(rec.minute, 2, 10, QChar('0')));
}

void CIEWidget::applyEpwRecord(const EpwDocument& doc, const EpwRecord& rec)
{
    const double midpoint = epwMidpointHour(rec.hour, rec.minute, doc.recordsPerHour);
    double dhi = rec.dhi;
    if (doc.recordsPerHour > 1) dhi *= doc.recordsPerHour; // 转为 W/m² 平均

    QDate date(rec.year, rec.month, rec.day);
    m_skyWidget->setDateTime(date, midpoint);
    m_skyWidget->setDiffuseHorizontalIrradiance(dhi);
}

double CIEWidget::epwMidpointHour(int hour, int minute, int recordsPerHour) const
{
    // EPW 时间标记为结束时刻，hour 1-24
    const double intervalHours = 1.0 / std::max(1, recordsPerHour);
    const double endHour = static_cast<double>(hour - 1) + static_cast<double>(minute) / 60.0;
    return endHour - intervalHours * 0.5;
}