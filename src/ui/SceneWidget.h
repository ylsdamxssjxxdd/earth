#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <osg/ref_ptr>
#include <osgViewer/CompositeViewer>

namespace earth::core {
class SimulationBootstrapper;
}

namespace earth::ui {

/**
 * @brief Qt与osgEarth之间的桥接窗口，负责驱动三维场景刷新。
 */
class SceneWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit SceneWidget(QWidget* parent = nullptr);

    /**
     * @brief 绑定场景引导器，使窗口能够访问场景根节点。
     */
    void setSimulation(core::SimulationBootstrapper* bootstrapper);

protected:
    /**
     * @brief 初始化OpenGL上下文并搭建osgViewer。
     */
    void initializeGL() override;

    /**
     * @brief 刷新渲染帧或在无场景时清屏。
     */
    void paintGL() override;

    /**
     * @brief 根据窗口尺寸调整摄像机视口。
     */
    void resizeGL(int w, int h) override;

private:
    void configureView(int width, int height);

    core::SimulationBootstrapper* m_bootstrapper = nullptr;
    osg::ref_ptr<osgViewer::CompositeViewer> m_viewer;
};

} // namespace earth::ui
