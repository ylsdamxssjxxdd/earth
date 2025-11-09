#include "ui/SceneWidget.h"

#include "core/SimulationBootstrapper.h"

#include <QDebug>
#include <QtGlobal>
#include <QEvent>
#include <QHideEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QVBoxLayout>
#include <algorithm>
#include <mutex>
#include <osg/Camera>
#include <osg/Vec4>
#include <osg/Viewport>
#include <osgGA/GUIEventAdapter>
#include <osgGA/StateSetManipulator>
#include <osgQt/GraphicsWindowQt>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>
#include <osgViewer/View>
#include <osgEarth/Common>
#include <osgEarth/EarthManipulator>
#include <osgEarth/ExampleResources>
#include <osgEarth/GeoData>
#include <osgEarth/MapNode>
#include <osgEarth/SpatialReference>
#include <osgEarth/Sky>

namespace earth::ui {
namespace {
constexpr int kDefaultWidth = 640;
constexpr int kDefaultHeight = 360;
constexpr double kNearPlane = 0.1;
constexpr double kFarPlane = 5e6;
constexpr int kFrameIntervalMs = 16;
} // namespace

SceneWidget::SceneWidget(QWidget* parent)
    : QWidget(parent)
    , m_viewer(new osgViewer::CompositeViewer())
    , m_view(new osgViewer::View()) {
    setMinimumSize(kDefaultWidth, kDefaultHeight);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    static std::once_flag initFlag;
    std::call_once(initFlag, [] {
        osgEarth::initialize();
    });

    m_viewer->setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);
    connect(&m_frameTimer, &QTimer::timeout, this, &SceneWidget::onFrame);
    m_frameTimer.setInterval(kFrameIntervalMs);
    m_frameTimer.start(kFrameIntervalMs);

    ensureGraphicsWindow();
    initializeViewer();
}

void SceneWidget::setSimulation(core::SimulationBootstrapper* bootstrapper) {
    m_bootstrapper = bootstrapper;
    m_lastAttachedSky = nullptr;
    applySceneData();
}

void SceneWidget::home() {
    if (!m_view.valid() || !m_graphicsWindow.valid()) {
        return;
    }

    if (auto* eq = m_graphicsWindow->getEventQueue()) {
        eq->keyPress(osgGA::GUIEventAdapter::KEY_Home);
        eq->keyRelease(osgGA::GUIEventAdapter::KEY_Home);
    }
}

void SceneWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (!m_frameTimer.isActive()) {
        m_frameTimer.start(kFrameIntervalMs);
    }
}

void SceneWidget::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    if (m_frameTimer.isActive()) {
        m_frameTimer.stop();
    }
    resetFrameStats();
}

void SceneWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateCamera(std::max(1, event->size().width()), std::max(1, event->size().height()));
}

bool SceneWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_glWidget && event->type() == QEvent::MouseMove) {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        double lon = 0.0;
        double lat = 0.0;
        double height = 0.0;
        if (computeGeoAt(mouseEvent->pos(), lon, lat, height)) {
            emit mouseGeoPositionChanged(lon, lat, height);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SceneWidget::onFrame() {
    if (!isVisible() || !m_viewerInitialized || !m_viewer.valid() || !m_graphicsWindow.valid()) {
        resetFrameStats();
        return;
    }

    if (!m_fpsTimer.isValid()) {
        m_fpsTimer.start();
        m_frameCounter = 0;
    }

    m_viewer->frame();
    updateFrameRateMetrics();
}

void SceneWidget::initializeViewer() {
    if (m_viewerInitialized || !m_viewer.valid() || !m_view.valid()) {
        return;
    }

    ensureGraphicsWindow();
    if (!m_graphicsWindow.valid()) {
        return;
    }

    m_view->setName("EmbeddedAirportView");
    m_view->setCameraManipulator(new osgEarth::Util::EarthManipulator());
    osgEarth::Util::MapNodeHelper().configureView(m_view.get());
    m_view->addEventHandler(new osgGA::StateSetManipulator(m_view->getCamera()->getOrCreateStateSet()));

    if (osg::Camera* camera = m_view->getCamera()) {
        camera->setGraphicsContext(m_graphicsWindow.get());
        camera->setClearColor(osg::Vec4(0.1f, 0.1f, 0.15f, 1.0f));
        camera->setDrawBuffer(GL_BACK);
        camera->setReadBuffer(GL_BACK);
    }

    m_viewer->addView(m_view.get());
    applySceneData();
    updateCamera(std::max(1, width()), std::max(1, height()));

    if (auto* eq = m_graphicsWindow->getEventQueue()) {
        const float dpr = currentDevicePixelRatio();
        const float mx = static_cast<float>(width()) * 0.5f * dpr;
        const float my = static_cast<float>(height()) * 0.5f * dpr;
        eq->mouseMotion(mx, my);
        qDebug() << "[SceneWidget] initialize mouseMotion at center (" << mx << "," << my << ")";
    }

    m_viewerInitialized = true;
}

void SceneWidget::ensureGraphicsWindow() {
    if (m_graphicsWindow.valid() && !m_glWidget.isNull()) {
        return;
    }

    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->windowDecoration = false;
    traits->doubleBuffer = true;
    traits->x = 0;
    traits->y = 0;
    traits->width = std::max(width(), kDefaultWidth);
    traits->height = std::max(height(), kDefaultHeight);
    traits->alpha = 8;
    traits->stencil = 8;
    traits->samples = 4;
    traits->sampleBuffers = traits->samples > 0 ? 1 : 0;
    traits->inheritedWindowData = new osgQt::GraphicsWindowQt::WindowData(nullptr, this);

    m_graphicsWindow = new osgQt::GraphicsWindowQt(traits.get(), this);
    if (!m_graphicsWindow.valid()) {
        qWarning() << "[SceneWidget] Failed to create osgQt::GraphicsWindowQt";
        return;
    }

    m_glWidget = m_graphicsWindow->getGLWidget();
    if (!m_glWidget) {
        qWarning() << "[SceneWidget] GraphicsWindowQt returned null GLWidget";
        return;
    }

    qDebug() << "[SceneWidget] GLWidget created, windowFlags=" << m_glWidget->windowFlags() << " parent="
             << m_glWidget->parent();

    m_glWidget->setFocusPolicy(Qt::StrongFocus);
    m_glWidget->setMouseTracking(true);
    m_glWidget->installEventFilter(this);
    setFocusProxy(m_glWidget);

    if (!layout()) {
        auto* newLayout = new QVBoxLayout(this);
        newLayout->setContentsMargins(0, 0, 0, 0);
        newLayout->setSpacing(0);
    }
    if (layout()->indexOf(m_glWidget) == -1) {
        layout()->addWidget(m_glWidget);
        qDebug() << "[SceneWidget] GLWidget added to layout, isWindow=" << m_glWidget->isWindow();
    }

    m_glWidget->show();
    qDebug() << "[SceneWidget] GLWidget show, isWindow=" << m_glWidget->isWindow()
             << " effective parent=" << m_glWidget->parentWidget();
}

void SceneWidget::applySceneData() {
    if (!m_view.valid()) {
        return;
    }

    if (m_bootstrapper != nullptr) {
        m_view->setSceneData(m_bootstrapper->sceneRoot());
    } else {
        m_view->setSceneData(nullptr);
        m_lastAttachedSky = nullptr;
        return;
    }

    configureEnvironment();
}

void SceneWidget::updateCamera(int width, int height) const {
    if (!m_graphicsWindow.valid() || !m_view.valid()) {
        return;
    }

    const float dpr = currentDevicePixelRatio();
    const int pixelW = std::max(1, static_cast<int>(width * dpr));
    const int pixelH = std::max(1, static_cast<int>(height * dpr));

    m_graphicsWindow->resized(0, 0, pixelW, pixelH);
    if (auto* queue = m_graphicsWindow->getEventQueue()) {
        queue->windowResize(0, 0, pixelW, pixelH);
        queue->syncWindowRectangleWithGraphicsContext();
    }

    if (osg::Camera* camera = m_view->getCamera()) {
        camera->setViewport(0, 0, pixelW, pixelH);
        if (pixelH > 0) {
            const double aspect = static_cast<double>(pixelW) / static_cast<double>(pixelH);
            camera->setProjectionMatrixAsPerspective(30.0, aspect, kNearPlane, kFarPlane);
        }
    }
}

void SceneWidget::updateFrameRateMetrics() {
    if (!m_fpsTimer.isValid()) {
        return;
    }

    ++m_frameCounter;
    const qint64 elapsedMs = m_fpsTimer.elapsed();
    if (elapsedMs < 250) {
        return;
    }

    const double elapsedSec = static_cast<double>(elapsedMs) / 1000.0;
    const double fps = elapsedSec > 0.0 ? static_cast<double>(m_frameCounter) / elapsedSec : 0.0;

    if (!qFuzzyCompare(1.0 + fps, 1.0 + m_lastReportedFps)) {
        emit frameRateChanged(fps);
        m_lastReportedFps = fps;
    }

    m_frameCounter = 0;
    m_fpsTimer.restart();
}

void SceneWidget::resetFrameStats() {
    m_frameCounter = 0;
    if (m_fpsTimer.isValid()) {
        m_fpsTimer.invalidate();
    }
    if (!qFuzzyCompare(1.0 + m_lastReportedFps, 1.0)) {
        m_lastReportedFps = 0.0;
        emit frameRateChanged(0.0);
    }
}

void SceneWidget::configureEnvironment() {
    if (!m_bootstrapper || !m_view.valid()) {
        return;
    }

    osgEarth::SkyNode* sky = m_bootstrapper->skyNode();
    if (!sky) {
        m_lastAttachedSky = nullptr;
        return;
    }

    if (sky != m_lastAttachedSky) {
        sky->attach(m_view.get(), 0);
        m_lastAttachedSky = sky;
    }
}

bool SceneWidget::computeGeoAt(const QPoint& pos, double& lon, double& lat, double& height) const {
    if (!m_view.valid()) {
        return false;
    }

    osgEarth::MapNode* mapNode = osgEarth::MapNode::findMapNode(m_view->getSceneData());
    if (!mapNode) {
        return false;
    }

    const double dpr = static_cast<double>(currentDevicePixelRatio());
    const double x = static_cast<double>(pos.x()) * dpr;
    const double y = static_cast<double>(pos.y()) * dpr;

    osg::ref_ptr<osgUtil::LineSegmentIntersector> lsi =
        new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW, x, y);
    osgUtil::IntersectionVisitor iv(lsi.get());

    osg::Camera* camera = m_view->getCamera();
    if (!camera) {
        return false;
    }

    camera->accept(iv);

    if (!lsi->containsIntersections()) {
        return false;
    }

    const auto& hit = *lsi->getIntersections().begin();
    const osg::Vec3d world = hit.getWorldIntersectPoint();

    const osgEarth::SpatialReference* mapSRS = mapNode->getMapSRS();
    if (!mapSRS) {
        return false;
    }

    osgEarth::GeoPoint mapPoint;
    mapPoint.fromWorld(mapSRS, world);

    const osgEarth::SpatialReference* geoSRS = mapSRS->getGeographicSRS();
    osgEarth::GeoPoint geoPoint;
    if (geoSRS) {
        mapPoint.transform(geoSRS, geoPoint);
    } else {
        geoPoint = mapPoint;
    }

    lon = geoPoint.x();
    lat = geoPoint.y();
    height = geoPoint.z();
    return true;
}

float SceneWidget::currentDevicePixelRatio() const {
    if (m_glWidget) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
        return static_cast<float>(m_glWidget->devicePixelRatioF());
#else
        return static_cast<float>(m_glWidget->devicePixelRatio());
#endif
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    return static_cast<float>(devicePixelRatioF());
#else
    return static_cast<float>(devicePixelRatio());
#endif
}

} // namespace earth::ui
