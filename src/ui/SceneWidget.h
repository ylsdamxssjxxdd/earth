#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QTimer>
#include <osg/ref_ptr>
#include <osgViewer/CompositeViewer>

class QHideEvent;
class QShowEvent;

namespace osgViewer {
class View;
class GraphicsWindowEmbedded;
}

namespace earth::core {
class SimulationBootstrapper;
}

namespace earth::ui {

/**
 * @brief ʹ��QOpenGLWidget ��osgEarth��Ⱦ���ڲ�ͬ���� Qt UI �о������Ӵ��ڡ�
 */
class SceneWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit SceneWidget(QWidget* parent = nullptr);

    /**
     * @brief ���ӻ����������� scene graph ���ڿ����滮ˢ�¡�
     */
    void setSimulation(core::SimulationBootstrapper* bootstrapper);

protected:
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

    core::SimulationBootstrapper* m_bootstrapper = nullptr;
    osg::ref_ptr<osgViewer::CompositeViewer> m_viewer;
    osg::ref_ptr<osgViewer::View> m_view;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> m_graphicsWindow;
    QTimer m_frameTimer;
    bool m_viewerInitialized = false;
};

} // namespace earth::ui

using SceneWidget = earth::ui::SceneWidget;
