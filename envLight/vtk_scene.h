#ifndef VTK_SCENE_WIDGET_H
#define VTK_SCENE_WIDGET_H

#include <QVTKOpenGLStereoWidget.h>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include "panorama_processor.h"
#include <QLabel>
class vtkRenderer;
class vtkSphereSource;
class vtkTexture;
class VTKSceneWidget : public QVTKOpenGLStereoWidget
{
    Q_OBJECT

public:
    explicit VTKSceneWidget(QWidget* parent = nullptr);
    ~VTKSceneWidget();

    // 设置全景图（仅用于生成透视视图，不在3D场景中显示纹理）
    void setPanorama(const Image& img);
    //void updateTexture();
    // 更新相机参数（同时更新3D示意和透视视图）
    void setCameraParameters(double cx, double cy, double cz,
        double yaw, double pitch, double roll,
        double hfov, double vfov, int outW, int outH);

signals:
    void perspectiveViewReady(const QImage& image); // 透视视图结果

private:
    void setupScene();              // 初始化3D场景
    void updatePerspective();       // 生成透视视图并发送信号
    void updateROIAndRay();         // 更新相机、射线、矩形框

    // VTK 对象
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkSphereSource> m_sphereSource;
    vtkSmartPointer<vtkActor> m_sphereActor;
    vtkSmartPointer<vtkActor> m_roiActor;
    vtkSmartPointer<vtkActor> m_cameraActor;
    vtkSmartPointer<vtkActor> m_rayActors[4];
    vtkSmartPointer<vtkActor> m_rectEdges[4];
    vtkSmartPointer<vtkTexture> m_texture;
    // 当前参数
    double m_cx, m_cy, m_cz;
    double m_yaw, m_pitch, m_roll;
    double m_hfov, m_vfov;
    int m_outW, m_outH;
    Image m_panorama;               // 全景图像（用于生成透视视图）
};
class PanoramaLabel : public QLabel
{
    Q_OBJECT
public:
    explicit PanoramaLabel(QWidget* parent = nullptr);
    void setPanoramaImage(const Image& img);
    void setCorners(const std::vector<QPointF>& corners); // 四个点按顺序，纹理坐标 (u,v) 范围 0~1
    void clearCorners();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap m_pixmap;
    bool m_hasCorners;
    std::vector<QPointF> m_corners; // 四个点
};

#endif // VTK_SCENE_WIDGET_H