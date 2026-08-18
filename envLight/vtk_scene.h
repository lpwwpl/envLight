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
class vtkImageData;
class vtkTransformTextureCoords;
class vtkAxesActor;
class VTKSceneWidget : public QVTKOpenGLStereoWidget
{
    Q_OBJECT

public:
    explicit VTKSceneWidget(QWidget* parent = nullptr);
    ~VTKSceneWidget();

    // 设置全景图：HDR float 用于计算，tone-mapped 预览同时覆盖到 VTK 球面
    void setPanorama(const HDRImage& img);
    // Set where North is located in the source equirectangular panorama.
    // 0 deg = left seam, 180 deg = image center. E/S/W follow every +90 deg.
    void setNorthDirectionDegrees(double northPanoramaDeg);
    // World representation of VTK scene vectors. Physical sky/solar models remain canonical ENU.
    void setCoordinateSystem(WorldCoordinateSystem coordinateSystem);
    WorldCoordinateSystem coordinateSystem() const { return m_coordinateSystem; }
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
    vtkSmartPointer<vtkImageData> m_textureImage;
    vtkSmartPointer<vtkTransformTextureCoords> m_textureTransform; // keep texture input alive across renders
    vtkSmartPointer<vtkAxesActor> m_axesActor;
    // 当前参数
    double m_cx, m_cy, m_cz;
    double m_yaw, m_pitch, m_roll;
    double m_hfov, m_vfov;
    double m_northDirectionDeg;
    WorldCoordinateSystem m_coordinateSystem;
    int m_outW, m_outH;
    HDRImage m_panorama;            // scene-linear HDR panorama for perspective generation
    Image m_panoramaDisplay;        // cached tone-mapped preview; never used for HDR calculations
};
class PanoramaLabel : public QLabel
{
    Q_OBJECT
public:
    explicit PanoramaLabel(QWidget* parent = nullptr);
    void setPanoramaImage(const Image& img);
    void setNorthDirectionDegrees(double northPanoramaDeg);
    void setCorners(const std::vector<QPointF>& corners); // 四个点按顺序，纹理坐标 (u,v) 范围 0~1
    void clearCorners();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPixmap m_pixmap;
    bool m_hasCorners;
    double m_northDirectionDeg;
    std::vector<QPointF> m_corners; // 四个点
};

#endif // VTK_SCENE_WIDGET_H