#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QTimer>
#include <osg/ref_ptr>
#include <osgViewer/CompositeViewer>

class QHideEvent;
class QShowEvent;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QPoint;

namespace osgEarth {
class SkyNode;
}

namespace osgViewer {
class View;
class GraphicsWindowEmbedded;
}

namespace earth::core {
class SimulationBootstrapper;
}

namespace earth::ui {

/**
 * @brief 基于 QOpenGLWidget 嵌入 osgEarth 场景的渲染窗口，负责桥接 Qt 事件与 osgViewer。
 */
class SceneWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit SceneWidget(QWidget* parent = nullptr);

    /**
     * @brief 安装仿真引导器，SceneWidget 会自动更新 scene graph 与环境设置。
     */
    void setSimulation(core::SimulationBootstrapper* bootstrapper);

    /**
     * @brief 恢复 EarthManipulator 的 Home 视点，便于回到初始俯瞰角度。
     */
    void home();

signals:
    /**
     * @brief 鼠标拾取到新的经纬高时发出的信号（单位：度/米）。
     */
    void mouseGeoPositionChanged(double lon, double lat, double height);

protected:
    // Qt 事件桥接到 osgViewer，开启鼠标/键盘操控
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onFrame();

private:
    void initializeViewer();
    void applySceneData();
    void updateCamera(int width, int height) const;
    /**
     * @brief 负责把 SkyNode 与嵌入式 viewer 对齐，确保星空/大气环境生效。
     */
    void configureEnvironment();
    /**
     * @brief 将屏幕坐标转换为经纬高，便于状态栏展示。
     */
    bool computeGeoAt(const QPoint& pos, double& lon, double& lat, double& height) const;

    core::SimulationBootstrapper* m_bootstrapper = nullptr;
    osg::ref_ptr<osgViewer::CompositeViewer> m_viewer;
    osg::ref_ptr<osgViewer::View> m_view;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> m_graphicsWindow;
    QTimer m_frameTimer;
    // 追踪鼠标拖拽状态以便自定义右键拖拽缩放行为
    Qt::MouseButtons m_buttonsDown{};
    QPoint m_lastPos{};
    bool m_zoomDragActive = false;
    bool m_viewerInitialized = false;
    const osgEarth::SkyNode* m_lastAttachedSky = nullptr;
};

} // namespace earth::ui

using SceneWidget = earth::ui::SceneWidget;

