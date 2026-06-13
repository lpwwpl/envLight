#include "vtk_scene.h"
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkLineSource.h>
#include <vtkTubeFilter.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyLine.h>
#include <cmath>
#include <QImage>
#include <vtkTexture.h>
#include <vtkImageData.h>
#include <vtkTextureMapToSphere.h>
#include <QPainter>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 辅助函数：纹理坐标转世界坐标（单位球面）
static void uvToWorld(double u, double v, double& x, double& y, double& z) {
    double theta = v * M_PI;
    double phi = u * 2.0 * M_PI;
    x = sin(theta) * cos(phi);
    y = cos(theta);
    z = sin(theta) * sin(phi);
}
// 射线与单位球面求交
static bool raySphereIntersection(const double origin[3], const double dir[3],
    double& hit_u, double& hit_v) {
    double a = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
    double b = 2.0 * (origin[0] * dir[0] + origin[1] * dir[1] + origin[2] * dir[2]);
    double c = origin[0] * origin[0] + origin[1] * origin[1] + origin[2] * origin[2] - 1.0;
    double disc = b * b - 4.0 * a * c;
    if (disc < 0.0) return false;
    double sqrt_disc = sqrt(disc);
    double t1 = (-b - sqrt_disc) / (2.0 * a);
    double t2 = (-b + sqrt_disc) / (2.0 * a);
    double t = (t1 > 1e-6) ? t1 : ((t2 > 1e-6) ? t2 : -1.0);
    if (t <= 1e-6) return false;
    double hit_x = origin[0] + t * dir[0];
    double hit_y = origin[1] + t * dir[1];
    double hit_z = origin[2] + t * dir[2];
    double theta = acos(hit_y);
    double phi = atan2(hit_x, hit_z);
    hit_u = (phi + M_PI) / (2.0 * M_PI);
    hit_v = theta / M_PI;
    return true;
}
// 射线与单位球面求交
static bool raySphereIntersection(const double origin[3], const double dir[3], double hit[3]) {
    double a = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
    double b = 2.0 * (origin[0] * dir[0] + origin[1] * dir[1] + origin[2] * dir[2]);
    double c = origin[0] * origin[0] + origin[1] * origin[1] + origin[2] * origin[2] - 1.0;
    double disc = b * b - 4.0 * a * c;
    if (disc < 0) return false;
    double sqrt_disc = sqrt(disc);
    double t1 = (-b - sqrt_disc) / (2.0 * a);
    double t2 = (-b + sqrt_disc) / (2.0 * a);
    double t = (t1 > 1e-6) ? t1 : ((t2 > 1e-6) ? t2 : -1.0);
    if (t <= 1e-6) return false;
    hit[0] = origin[0] + t * dir[0];
    hit[1] = origin[1] + t * dir[1];
    hit[2] = origin[2] + t * dir[2];
    return true;
}

// 创建线段（带管状效果）
static vtkSmartPointer<vtkActor> createLineSegment(const double p1[3], const double p2[3],
    double r, double g, double b, double width = 2.0) {
    vtkSmartPointer<vtkLineSource> line = vtkSmartPointer<vtkLineSource>::New();
    line->SetPoint1(p1[0], p1[1], p1[2]);
    line->SetPoint2(p2[0], p2[1], p2[2]);
    vtkSmartPointer<vtkTubeFilter> tube = vtkSmartPointer<vtkTubeFilter>::New();
    tube->SetInputConnection(line->GetOutputPort());
    tube->SetRadius(0.008);
    tube->SetNumberOfSides(6);
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(tube->GetOutputPort());
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(r, g, b);
    return actor;
}

// 创建小球体
static vtkSmartPointer<vtkActor> createSphereActor(double cx, double cy, double cz,
    double radius, double r, double g, double b) {
    vtkSmartPointer<vtkSphereSource> sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->SetCenter(cx, cy, cz);
    sphere->SetRadius(radius);
    sphere->SetThetaResolution(20);
    sphere->SetPhiResolution(20);
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphere->GetOutputPort());
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(r, g, b);
    return actor;
}

// 创建球面上的矩形框（由经纬线构成）
static vtkSmartPointer<vtkActor> createSphereRectangle(double u0, double u1, double v0, double v1,
    double r, double g, double b, double width = 2.0) {
    const int steps = 40;
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

    auto addLine = [&](double u_start, double u_end, double v_fixed) {
        vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
        for (int i = 0; i <= steps; ++i) {
            double t = (double)i / steps;
            double u = u_start + t * (u_end - u_start);
            double x, y, z;
            uvToWorld(u, v_fixed, x, y, z);
            vtkIdType id = points->InsertNextPoint(x, y, z);
            polyLine->GetPointIds()->InsertNextId(id);
        }
        lines->InsertNextCell(polyLine);
    };
    auto addVLine = [&](double u_fixed, double v_start, double v_end) {
        vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
        for (int i = 0; i <= steps; ++i) {
            double t = (double)i / steps;
            double v = v_start + t * (v_end - v_start);
            double x, y, z;
            uvToWorld(u_fixed, v, x, y, z);
            vtkIdType id = points->InsertNextPoint(x, y, z);
            polyLine->GetPointIds()->InsertNextId(id);
        }
        lines->InsertNextCell(polyLine);
    };

    if (u0 > u1) { // 跨越经度边界
        addLine(u0, 1.0, v0); addLine(u0, 1.0, v1);
        addVLine(u0, v0, v1); addVLine(1.0, v0, v1);
        addLine(0.0, u1, v0); addLine(0.0, u1, v1);
        addVLine(0.0, v0, v1); addVLine(u1, v0, v1);
    }
    else {
        addLine(u0, u1, v0); addLine(u0, u1, v1);
        addVLine(u0, v0, v1); addVLine(u1, v0, v1);
    }

    vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetLines(lines);
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polyData);
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(r, g, b);
    actor->GetProperty()->SetLineWidth(width);
    return actor;
}

// ==================== VTKSceneWidget 实现 ====================
VTKSceneWidget::VTKSceneWidget(QWidget* parent)
    : QVTKOpenGLStereoWidget(parent)
    , m_cx(0.5), m_cy(0.2), m_cz(0.3)
    , m_yaw(30), m_pitch(20), m_roll(0)
    , m_hfov(90), m_vfov(60), m_outW(800), m_outH(600)
{
    setupScene();
}

VTKSceneWidget::~VTKSceneWidget() = default;

void VTKSceneWidget::setupScene() {
    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->SetBackground(0.1, 0.1, 0.2);
    this->renderWindow()->AddRenderer(m_renderer);

    // 球体（半透明，不带纹理）
    m_sphereSource = vtkSmartPointer<vtkSphereSource>::New();
    m_sphereSource->SetRadius(1.0);
    m_sphereSource->SetThetaResolution(100);
    m_sphereSource->SetPhiResolution(100);
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(m_sphereSource->GetOutputPort());
    m_sphereActor = vtkSmartPointer<vtkActor>::New();

    // 生成纹理坐标
    //vtkSmartPointer<vtkTextureMapToSphere> textureMap = vtkSmartPointer<vtkTextureMapToSphere>::New();
    //textureMap->SetInputConnection(m_sphereSource->GetOutputPort());
    //textureMap->Update();
    //vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    //mapper->SetInputConnection(textureMap->GetOutputPort());
    m_sphereActor->SetMapper(mapper);
    m_sphereActor->GetProperty()->SetColor(1.0, 1.0, 1.0);
    m_sphereActor->GetProperty()->SetOpacity(0.3);
    m_renderer->AddActor(m_sphereActor);




    // 创建纹理对象
    m_texture = vtkSmartPointer<vtkTexture>::New();
    m_texture->InterpolateOn();          // 平滑插值
    m_sphereActor->SetTexture(m_texture);
    m_renderer->AddActor(m_sphereActor);

    // 线框球体（增强立体感）
    vtkSmartPointer<vtkSphereSource> wireSphere = vtkSmartPointer<vtkSphereSource>::New();
    wireSphere->SetRadius(1.01);
    wireSphere->SetThetaResolution(48);
    wireSphere->SetPhiResolution(24);
    vtkSmartPointer<vtkPolyDataMapper> wireMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    wireMapper->SetInputConnection(wireSphere->GetOutputPort());
    vtkSmartPointer<vtkActor> wireActor = vtkSmartPointer<vtkActor>::New();
    wireActor->SetMapper(wireMapper);
    wireActor->GetProperty()->SetColor(0.9, 0.9, 0.9);
    wireActor->GetProperty()->SetRepresentationToWireframe();
    m_renderer->AddActor(wireActor);

    // 坐标轴
    vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
    axes->SetTotalLength(1.2, 1.2, 1.2);
    m_renderer->AddActor(axes);

    // 相机视角
    m_renderer->GetActiveCamera()->SetPosition(2.5, 1.5, 2.0);
    m_renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
    m_renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
    m_renderer->ResetCameraClippingRange();

    // 初始化动态 actors
    m_roiActor = nullptr;
    m_cameraActor = nullptr;
    for (int i = 0; i < 4; ++i) {
        m_rayActors[i] = nullptr;
        m_rectEdges[i] = nullptr;
    }

    // 初始更新一次（使用默认参数）
    updateROIAndRay();
    updatePerspective();
}

void VTKSceneWidget::setPanorama(const Image & img) {
    m_panorama = img;
    updatePerspective();
}
void VTKSceneWidget::setCameraParameters(double cx, double cy, double cz,
    double yaw, double pitch, double roll,
    double hfov, double vfov, int outW, int outH) {
    m_cx = cx; m_cy = cy; m_cz = cz;
    m_yaw = yaw; m_pitch = pitch; m_roll = roll;
    m_hfov = hfov; m_vfov = vfov;
    m_outW = outW; m_outH = outH;

    updateROIAndRay();
    updatePerspective();

    this->renderWindow()->Render();
}

void VTKSceneWidget::updatePerspective() {
    if (m_panorama.width == 0) return;

    Image perspective = PanoramaProcessor::perspectiveFromPanorama(
        m_panorama, m_cx, m_cy, m_cz, m_yaw, m_pitch, m_roll,
        m_hfov, m_vfov, m_outW, m_outH, 2); // 抗锯齿 2x2

    QImage qimg(perspective.width, perspective.height, QImage::Format_RGB888);
    for (int y = 0; y < perspective.height; ++y) {
        for (int x = 0; x < perspective.width; ++x) {
            const sRGB& c = perspective.at(x, y);
            qimg.setPixel(x, y, qRgb(c.r, c.g, c.b));
        }
    }


    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    imageData->SetDimensions(m_panorama.width, m_panorama.height, 1);
    imageData->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

    for (int y = 0; y < m_panorama.height; ++y) {
        for (int x = 0; x < m_panorama.width; ++x) {
            const sRGB& c = m_panorama.at(x, y);
            unsigned char* pixel = static_cast<unsigned char*>(imageData->GetScalarPointer(x, y, 0));
            pixel[0] = c.r;
            pixel[1] = c.g;
            pixel[2] = c.b;
        }
    }

    m_texture->SetInputData(imageData);
    m_texture->Update();   // 必须调用，纹理才会生效

    // 刷新渲染
    this->renderWindow()->Render();

    emit perspectiveViewReady(qimg);
}

void VTKSceneWidget::updateROIAndRay() {
    // 计算旋转矩阵
    double yaw_rad = m_yaw * M_PI / 180.0;
    double pitch_rad = m_pitch * M_PI / 180.0;
    double roll_rad = m_roll * M_PI / 180.0;
    double cy = cos(yaw_rad), sy = sin(yaw_rad);
    double cp = cos(pitch_rad), sp = sin(pitch_rad);
    double cr = cos(roll_rad), sr = sin(roll_rad);
    double R[3][3] = {
        { cy * cp,  cy * sp * sr - sy * cr,  cy * sp * cr + sy * sr },
        { sy * cp,  sy * sp * sr + cy * cr,  sy * sp * cr - cy * sr },
        { -sp,    cp * sr,              cp * cr }
    };

    // 焦距
    double hfov_rad = m_hfov * M_PI / 180.0;
    double focalX = (m_outW / 2.0) / tan(hfov_rad / 2.0);
    double focalY = (m_vfov > 0) ? (m_outH / 2.0) / tan(m_vfov * M_PI / 180.0 / 2.0)
        : focalX * (double(m_outH) / m_outW);
    double halfW = m_outW / 2.0;
    double halfH = m_outH / 2.0;
    double origin[3] = { m_cx, m_cy, m_cz };

    // 四个角点像素坐标
    std::pair<double, double> corners[4] = { {0,0}, {m_outW - 1,0}, {0,m_outH - 1}, {m_outW - 1,m_outH - 1} };
    double hitPoints[4][3];
    bool ok = true;
    for (int i = 0; i < 4; ++i) {
        double x = corners[i].first;
        double y = corners[i].second;
        double nx = (x - halfW) / focalX;
        double ny = (y - halfH) / focalY;
        double nz = 1.0;
        double len = sqrt(nx * nx + ny * ny + nz * nz);
        if (len < 1e-6) { ok = false; break; }
        nx /= len; ny /= len; nz /= len;

        double wx = R[0][0] * nx + R[0][1] * ny + R[0][2] * nz;
        double wy = R[1][0] * nx + R[1][1] * ny + R[1][2] * nz;
        double wz = R[2][0] * nx + R[2][1] * ny + R[2][2] * nz;
        len = sqrt(wx * wx + wy * wy + wz * wz);
        if (len < 1e-6) { ok = false; break; }
        wx /= len; wy /= len; wz /= len;

        double dir[3] = { wx, wy, wz };
        if (!raySphereIntersection(origin, dir, hitPoints[i])) {
            ok = false;
            break;
        }
    }

    // 移除旧的动态 actors
    if (m_cameraActor) m_renderer->RemoveActor(m_cameraActor);
    for (int i = 0; i < 4; ++i) {
        if (m_rayActors[i]) m_renderer->RemoveActor(m_rayActors[i]);
        if (m_rectEdges[i]) m_renderer->RemoveActor(m_rectEdges[i]);
    }
    if (m_roiActor) m_renderer->RemoveActor(m_roiActor);

    // 相机小球体
    m_cameraActor = createSphereActor(m_cx, m_cy, m_cz, 0.05, 1.0, 0.2, 0.2);
    m_renderer->AddActor(m_cameraActor);


    if (ok) {
        // 射线（绿色）
        for (int i = 0; i < 4; ++i) {
            m_rayActors[i] = createLineSegment(origin, hitPoints[i], 0.2, 1.0, 0.2, 2.0);
            m_renderer->AddActor(m_rayActors[i]);
        }
        // 角点连线矩形（绿色）
        int order[5] = { 0,1,3,2,0 };
        for (int i = 0; i < 4; ++i) {
            int idx1 = order[i];
            int idx2 = order[i + 1];
            m_rectEdges[i] = createLineSegment(hitPoints[idx1], hitPoints[idx2], 0.2, 1.0, 0.2, 2.5);
            m_renderer->AddActor(m_rectEdges[i]);
        }
    }

    // 刷新渲染
    this->renderWindow()->Render();
}


PanoramaLabel::PanoramaLabel(QWidget* parent) : QLabel(parent), m_hasCorners(false)
{
    setAlignment(Qt::AlignCenter);
    setMinimumSize(400, 200);
    setStyleSheet("border: 1px solid gray; background-color: #2a2a2a;");
}

void PanoramaLabel::setPanoramaImage(const Image& img)
{
    QImage qimg(img.width, img.height, QImage::Format_RGB888);
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            const sRGB& c = img.at(x, y);
            qimg.setPixel(x, y, qRgb(c.r, c.g, c.b));
        }
    }
    m_pixmap = QPixmap::fromImage(qimg);
    update();
}

void PanoramaLabel::setCorners(const std::vector<QPointF>& corners)
{
    if (corners.size() >= 4) {
        m_corners = corners;
        m_hasCorners = true;
    }
    else {
        m_hasCorners = false;
    }
    update();
}

void PanoramaLabel::clearCorners()
{
    m_hasCorners = false;
    update();
}

void PanoramaLabel::paintEvent(QPaintEvent* event)
{
    if (m_pixmap.isNull()) {
        QLabel::paintEvent(event);
        return;
    }

    QPainter painter(this);
    // 缩放以适应控件，保持宽高比
    QPixmap scaled = m_pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    int x = (width() - scaled.width()) / 2;
    int y = (height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);

    if (m_hasCorners && m_corners.size() >= 3) {
        QPolygonF poly;
        double imgW = scaled.width(), imgH = scaled.height();
        for (const auto& uv : m_corners) {
            poly << QPointF(uv.x() * imgW + x, uv.y() * imgH + y);
        }
        painter.setPen(QPen(Qt::red, 2));
        painter.drawPolyline(poly); // 首尾不自动闭合，可以根据需要添加闭合线
        if (poly.size() > 2) painter.drawLine(poly.last(), poly.first());
    }
}