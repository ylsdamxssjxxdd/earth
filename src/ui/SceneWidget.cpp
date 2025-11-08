#include "ui/SceneWidget.h"

#include "core/SimulationBootstrapper.h"

#include <QHideEvent>
#include <QShowEvent>
#include <algorithm>
#include <mutex>
#include <osg/Camera>
#include <osg/Vec4>
#include <osg/Viewport>
#include <osgEarth/Common>
#include <osgEarth/EarthManipulator>
#include <osgEarth/ExampleResources>
#include <osgGA/StateSetManipulator>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/View>

namespace earth::ui {
namespace {
constexpr int kDefaultWidth = 640;
constexpr int kDefaultHeight = 360;
constexpr double kNearPlane = 0.1;
constexpr double kFarPlane = 5e6;
constexpr int kFrameIntervalMs = 16;
} // namespace

SceneWidget::SceneWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_viewer(new osgViewer::CompositeViewer())
    , m_view(new osgViewer::View()) {
    setMinimumSize(kDefaultWidth, kDefaultHeight);
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
    setFocusPolicy(Qt::StrongFocus);

    static std::once_flag initFlag;
    std::call_once(initFlag, [] {
        osgEarth::initialize();
    });

    m_viewer->setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);
    connect(&m_frameTimer, &QTimer::timeout, this, &SceneWidget::onFrame);
    m_frameTimer.setInterval(kFrameIntervalMs);
}

void SceneWidget::setSimulation(core::SimulationBootstrapper* bootstrapper) {
    m_bootstrapper = bootstrapper;
    applySceneData();
}

void SceneWidget::initializeGL() {
    initializeOpenGLFunctions();
    initializeViewer();
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
    updateCamera(std::max(1, w), std::max(1, h));
}

void SceneWidget::showEvent(QShowEvent* event) {
    QOpenGLWidget::showEvent(event);
    if (!m_frameTimer.isActive()) {
        m_frameTimer.start();
    }
}

void SceneWidget::hideEvent(QHideEvent* event) {
    QOpenGLWidget::hideEvent(event);
    if (m_frameTimer.isActive()) {
        m_frameTimer.stop();
    }
}

void SceneWidget::onFrame() {
    if (isVisible()) {
        update();
    }
}

void SceneWidget::initializeViewer() {
    if (m_viewerInitialized || !m_viewer.valid() || !m_view.valid()) {
        return;
    }

    m_view->setName("EmbeddedAirportView");
    m_view->setCameraManipulator(new osgEarth::Util::EarthManipulator());
    osgEarth::Util::MapNodeHelper().configureView(m_view.get());
    m_view->addEventHandler(new osgGA::StateSetManipulator(m_view->getCamera()->getOrCreateStateSet()));

    m_graphicsWindow = new osgViewer::GraphicsWindowEmbedded(0, 0, width(), height());
    osg::Camera* camera = m_view->getCamera();
    if (camera != nullptr) {
        camera->setGraphicsContext(m_graphicsWindow.get());
        camera->setClearColor(osg::Vec4(0.1f, 0.1f, 0.15f, 1.0f));
    }

    m_viewer->addView(m_view.get());
    applySceneData();
    updateCamera(std::max(1, width()), std::max(1, height()));
    m_viewerInitialized = true;
}

void SceneWidget::applySceneData() {
    if (!m_view.valid()) {
        return;
    }

    if (m_bootstrapper != nullptr) {
        m_view->setSceneData(m_bootstrapper->sceneRoot());
    } else {
        m_view->setSceneData(nullptr);
    }
}

void SceneWidget::updateCamera(int width, int height) const {
    if (!m_graphicsWindow.valid() || !m_view.valid()) {
        return;
    }

    m_graphicsWindow->resized(0, 0, width, height);
    if (auto* queue = m_graphicsWindow->getEventQueue()) {
        queue->windowResize(0, 0, width, height);
    }

    if (osg::Camera* camera = m_view->getCamera()) {
        camera->setViewport(0, 0, width, height);
        const double aspect = static_cast<double>(width) / static_cast<double>(height);
        camera->setProjectionMatrixAsPerspective(30.0, aspect, kNearPlane, kFarPlane);
    }
}

} // namespace earth::ui
