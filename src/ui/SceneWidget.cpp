#include "ui/SceneWidget.h"

#include "core/SimulationBootstrapper.h"

#include <osgGA/TrackballManipulator>
#include <osgViewer/View>

namespace earth::ui {

SceneWidget::SceneWidget(QWidget* parent)
    : QOpenGLWidget(parent) {
    setMinimumSize(640, 360);
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
}

void SceneWidget::setSimulation(core::SimulationBootstrapper* bootstrapper) {
    m_bootstrapper = bootstrapper;
    if (m_viewer.valid()) {
        configureView(width(), height());
    }
    update();
}

void SceneWidget::initializeGL() {
    initializeOpenGLFunctions();
    m_viewer = new osgViewer::CompositeViewer();
    m_viewer->setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);
    configureView(width(), height());
}

void SceneWidget::paintGL() {
    if (m_viewer.valid()) {
        m_viewer->frame();
    } else {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void SceneWidget::resizeGL(int w, int h) {
    configureView(w, h);
}

void SceneWidget::configureView(int width, int height) {
    if (!m_viewer.valid()) {
        return;
    }

    if (m_viewer->getNumViews() == 0) {
        osg::ref_ptr<osgViewer::View> view = new osgViewer::View();
        view->setName("AirportPreviewView");
        view->setCameraManipulator(new osgGA::TrackballManipulator());
        if (m_bootstrapper != nullptr) {
            view->setSceneData(m_bootstrapper->sceneRoot());
        }
        m_viewer->addView(view.get());
    }

    for (unsigned int i = 0; i < m_viewer->getNumViews(); ++i) {
        osgViewer::View* view = m_viewer->getView(i);
        if (view != nullptr && view->getCamera() != nullptr) {
            view->getCamera()->setViewport(0, 0, width, height);
        }
    }
}

} // namespace earth::ui
