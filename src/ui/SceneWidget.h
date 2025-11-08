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

namespace osgViewer {
class View;
class GraphicsWindowEmbedded;
}

namespace earth::core {
class SimulationBootstrapper;
}

namespace earth::ui {


class SceneWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit SceneWidget(QWidget* parent = nullptr);


    void setSimulation(core::SimulationBootstrapper* bootstrapper);

    /**
     * @brief 重置视图到Home视点（EarthManipulator的home位置）。
     */
    void home();

signals:
    /**
     * @brief 鼠标地理位置变化信号：经度(lon)、纬度(lat)、高程(height, 米)。
     */
    void mouseGeoPositionChanged(double lon, double lat, double height);

protected:
    // Qt事件桥接到osgViewer，开启鼠标/键盘操控
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
     * @brief 将屏幕坐标拾取为地理坐标。
     * @return 成功则返回true并输出经纬高
     */
    bool computeGeoAt(const QPoint& pos, double& lon, double& lat, double& height) const;

    core::SimulationBootstrapper* m_bootstrapper = nullptr;
    osg::ref_ptr<osgViewer::CompositeViewer> m_viewer;
    osg::ref_ptr<osgViewer::View> m_view;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> m_graphicsWindow;
    QTimer m_frameTimer;
    // 追踪鼠标拖拽状态以定制右键缩放行为（将右拖动转译为滚轮缩放，避免跳变）
    Qt::MouseButtons m_buttonsDown{};
    QPoint m_lastPos{};
    bool m_zoomDragActive = false;
    bool m_viewerInitialized = false;
};

} // namespace earth::ui

using SceneWidget = earth::ui::SceneWidget;
