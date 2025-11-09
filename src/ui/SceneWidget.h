#pragma once

#include <QElapsedTimer>
#include <QPointer>
#include <QTimer>
#include <QWidget>
#include <osg/ref_ptr>
#include <osgViewer/CompositeViewer>

class QHideEvent;
class QShowEvent;
class QResizeEvent;
class QMouseEvent;
class QPaintEvent;
class QEvent;
class QPoint;

namespace osgEarth {
class SkyNode;
}

namespace osgViewer {
class View;
}

namespace osgQt {
class GraphicsWindowQt;
class GLWidget;
}

namespace earth::core {
class SimulationBootstrapper;
}

namespace earth::ui {

/**
 * @brief 基于 osgQt::GraphicsWindowQt 的 osgEarth 场景窗口，负责在 Qt UI 中嵌入三维视图并桥接交互。
 */
class SceneWidget : public QWidget {
    Q_OBJECT

public:
    explicit SceneWidget(QWidget* parent = nullptr);

    /**
     * @brief 注入仿真初始化器，SceneWidget 会自动挂接场景与环境配置。
     */
    void setSimulation(core::SimulationBootstrapper* bootstrapper);

    /**
     * @brief 触发 EarthManipulator 的 Home 行为，便于回到初始观测点。
     */
    void home();

signals:
    /**
     * @brief 鼠标拾取新的经纬度时发出信号，单位为度/米。
     */
    void mouseGeoPositionChanged(double lon, double lat, double height);
    /**
     * @brief 渲染帧率统计更新时发出信号，单位为 FPS。
     */
    void frameRateChanged(double fps);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onFrame();

private:
    void initializeViewer();
    void ensureGraphicsWindow();
    void applySceneData();
    void updateCamera(int width, int height) const;
    void updateFrameRateMetrics();
    void resetFrameStats();
    /**
     * @brief 将 SkyNode 等环境节点装载进 viewer，确保昼夜/大气等效果正常。
     */
    void configureEnvironment();
    /**
     * @brief 将屏幕坐标转换为经纬度，供状态栏等模块展示。
     */
    bool computeGeoAt(const QPoint& pos, double& lon, double& lat, double& height) const;
    float currentDevicePixelRatio() const;

    core::SimulationBootstrapper* m_bootstrapper = nullptr;
    osg::ref_ptr<osgViewer::CompositeViewer> m_viewer;
    osg::ref_ptr<osgViewer::View> m_view;
    osg::ref_ptr<osgQt::GraphicsWindowQt> m_graphicsWindow;
    QPointer<osgQt::GLWidget> m_glWidget;
    QTimer m_frameTimer;
    bool m_viewerInitialized = false;
    const osgEarth::SkyNode* m_lastAttachedSky = nullptr;
    QElapsedTimer m_fpsTimer;
    int m_frameCounter = 0;
    double m_lastReportedFps = 0.0;
};

} // namespace earth::ui

using SceneWidget = earth::ui::SceneWidget;
