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
#include <osgEarth/Sky>
#include <osgGA/StateSetManipulator>
#include <osgViewer/GraphicsWindow>
#include <osgViewer/View>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QInputEvent>
#include <QDebug>

// 将Qt修饰键映射到OSG事件队列，参考 osgQt GraphicsWindowQt 的实现
static inline void setKeyboardModifiers(osgViewer::GraphicsWindow* gw, QInputEvent* event) {
    if (!gw || !gw->getEventQueue()) return;
    int modkey = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
    unsigned int mask = 0;
    if (modkey & Qt::ShiftModifier) mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
    if (modkey & Qt::ControlModifier) mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
    if (modkey & Qt::AltModifier) mask |= osgGA::GUIEventAdapter::MODKEY_ALT;
    gw->getEventQueue()->getCurrentEventState()->setModKeyMask(mask);
}
#include <osgGA/GUIEventAdapter>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>
#include <osgEarth/MapNode>
#include <osgEarth/SpatialReference>
#include <osgEarth/GeoData>

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
    setMouseTracking(true);

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
    m_lastAttachedSky = nullptr;
    applySceneData();
}

void SceneWidget::home() {
    if (!m_view.valid()) {
        return;
    }
    if (m_graphicsWindow.valid()) {
        if (auto* eq = m_graphicsWindow->getEventQueue()) {
            eq->keyPress(osgGA::GUIEventAdapter::KEY_Home);
            eq->keyRelease(osgGA::GUIEventAdapter::KEY_Home);
        }
    }
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

// ========== Qt -> osgViewer 事件桥接 ==========
namespace {
inline int toOsgButton(Qt::MouseButton b) {
    switch (b) {
    case Qt::LeftButton:   return 1;
    case Qt::MiddleButton: return 2;
    case Qt::RightButton:  return 3; // 保持EarthManipulator右键缩放逻辑，避免跳跃
    default:               return 0;
    }
}

/**
 * @brief 将Qt按键值映射为osgGA::GUIEventAdapter按键编码
 */
inline int mapQtKeyToOsg(int k) {
    using osgGA::GUIEventAdapter;
    switch (k) {
    case Qt::Key_Left:      return GUIEventAdapter::KEY_Left;
    case Qt::Key_Right:     return GUIEventAdapter::KEY_Right;
    case Qt::Key_Up:        return GUIEventAdapter::KEY_Up;
    case Qt::Key_Down:      return GUIEventAdapter::KEY_Down;
    case Qt::Key_PageUp:    return GUIEventAdapter::KEY_Page_Up;
    case Qt::Key_PageDown:  return GUIEventAdapter::KEY_Page_Down;
    case Qt::Key_Home:      return GUIEventAdapter::KEY_Home;
    case Qt::Key_End:       return GUIEventAdapter::KEY_End;
    case Qt::Key_Space:     return ' ';
    case Qt::Key_Plus:      return '+';
    case Qt::Key_Minus:     return '-';
    default:
        // 直接传递可打印ASCII
        if (k >= 0x20 && k <= 0x7e) return k;
        return 0;
    }
}
}

void SceneWidget::mousePressEvent(QMouseEvent* event) {
    if (m_graphicsWindow.valid()) {
        if (auto* eq = m_graphicsWindow->getEventQueue()) {
            const float dpr = static_cast<float>(this->devicePixelRatioF());
            const float x = static_cast<float>(event->pos().x() * dpr);
            const float y = static_cast<float>(event->pos().y() * dpr);
            setKeyboardModifiers(m_graphicsWindow.get(), event);
            const int btn = toOsgButton(event->button());
            if (btn != 0) {
                eq->mouseButtonPress(x, y, btn);
            }
            // 记录当前按键与位置（用于右键拖拽平滑缩放）
            m_buttonsDown = event->buttons();
            m_zoomDragActive = (event->button() == Qt::RightButton);
            m_lastPos = event->pos();
        }
    }
    setFocus();
    event->accept();
    update();
}

void SceneWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (m_graphicsWindow.valid()) {
        if (auto* eq = m_graphicsWindow->getEventQueue()) {
            const float dpr = static_cast<float>(this->devicePixelRatioF());
            const float x = static_cast<float>(event->pos().x() * dpr);
            const float y = static_cast<float>(event->pos().y() * dpr);
            setKeyboardModifiers(m_graphicsWindow.get(), event);
            const int btn = toOsgButton(event->button());
            if (btn != 0) {
                eq->mouseButtonRelease(x, y, btn);
            }
            // 更新按键状态
            m_buttonsDown = event->buttons();
            m_zoomDragActive = false;
            m_lastPos = event->pos();
        }
    }
    event->accept();
    update();
}

void SceneWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_graphicsWindow.valid()) {
        if (auto* eq = m_graphicsWindow->getEventQueue()) {
            const float dpr = static_cast<float>(this->devicePixelRatioF());
            const float x = static_cast<float>(event->pos().x() * dpr);
            const float y = static_cast<float>(event->pos().y() * dpr);
            setKeyboardModifiers(m_graphicsWindow.get(), event);
            if (m_zoomDragActive) {
                const int dy = event->pos().y() - m_lastPos.y();
                const int step = 24; // 每24像素触发一次缩放步进，避免过快
                int steps = dy / step;
                if (steps != 0) {
                    const auto dir = (steps > 0)
                        ? osgGA::GUIEventAdapter::SCROLL_DOWN
                        : osgGA::GUIEventAdapter::SCROLL_UP;
                    steps = std::abs(steps);
                    for (int i = 0; i < steps; ++i) {
                        eq->mouseScroll(dir);
                    }
                    // 累计消耗的像素，剩余偏移留待下一次
                    m_lastPos.setY(m_lastPos.y() + ((dy > 0) ? steps * step : -steps * step));
                }
            } else {
                eq->mouseMotion(x, y);
                m_lastPos = event->pos();
            }
        }
    }

    // 计算并广播鼠标处经纬高
    double lon = 0.0, lat = 0.0, h = 0.0;
    if (computeGeoAt(event->pos(), lon, lat, h)) {
        emit mouseGeoPositionChanged(lon, lat, h);
    }

    event->accept();
    update();
}

void SceneWidget::wheelEvent(QWheelEvent* event) {
    if (m_graphicsWindow.valid()) {
        if (auto* eq = m_graphicsWindow->getEventQueue()) {
            setKeyboardModifiers(m_graphicsWindow.get(), event);
            const int dy = event->angleDelta().y();
            if (dy != 0) {
                eq->mouseScroll(dy > 0
                                    ? osgGA::GUIEventAdapter::SCROLL_UP
                                    : osgGA::GUIEventAdapter::SCROLL_DOWN);
            }
        }
    }
    event->accept();
    update();
}

void SceneWidget::keyPressEvent(QKeyEvent* event) {
    // 为避免方向键首按产生“跳变”，将方向键映射为一次轻微的鼠标拖拽（左键）而不是直接发送键盘事件，
    // 这样可稳定由 EarthManipulator 接管旋转/平移逻辑。
    const int pixelStep = 2; // 单步像素（减小步进，降低每次首步的视角变化幅度）
    if (m_graphicsWindow.valid()) {
        if (auto* eq = m_graphicsWindow->getEventQueue()) {
            setKeyboardModifiers(m_graphicsWindow.get(), event);
            const float dpr = static_cast<float>(this->devicePixelRatioF());
            const float sx = static_cast<float>(m_lastPos.x() * dpr);
            const float sy = static_cast<float>(m_lastPos.y() * dpr);

            bool handled = false;
            QPoint newPos = m_lastPos;

            switch (event->key()) {
            case Qt::Key_Left:
                newPos.setX(m_lastPos.x() - pixelStep);
                handled = true;
                break;
            case Qt::Key_Right:
                newPos.setX(m_lastPos.x() + pixelStep);
                handled = true;
                break;
            case Qt::Key_Up:
                newPos.setY(m_lastPos.y() - pixelStep);
                handled = true;
                break;
            case Qt::Key_Down:
                newPos.setY(m_lastPos.y() + pixelStep);
                handled = true;
                break;
            default:
                // 非方向键：PageUp/PageDown映射为滚轮，其他按原逻辑发送键盘事件
                if (event->key() == Qt::Key_PageUp) {
                    eq->mouseScroll(osgGA::GUIEventAdapter::SCROLL_UP);
                } else if (event->key() == Qt::Key_PageDown) {
                    eq->mouseScroll(osgGA::GUIEventAdapter::SCROLL_DOWN);
                } else {
                    const int code = mapQtKeyToOsg(event->key());
                    if (code != 0) {
                        // qDebug() << "[KeyPress]" << event->key()
                        //          << " autoRepeat=" << event->isAutoRepeat()
                        //          << " lastMouse(" << m_lastPos.x() << "," << m_lastPos.y() << ")";
                        eq->keyPress(code);
                    }
                }
                break;
            }

            if (handled) {
                const float ex = static_cast<float>(newPos.x() * dpr);
                const float ey = static_cast<float>(newPos.y() * dpr);
                // qDebug() << "[ArrowStep]" << event->key()
                //          << " drag (" << m_lastPos.x() << "," << m_lastPos.y()
                //          << ") -> (" << newPos.x() << "," << newPos.y() << ")";
                // 合成一次轻微左键拖拽：按下->移动->抬起
                eq->mouseButtonPress(sx, sy, 1);
                eq->mouseMotion(ex, ey);
                eq->mouseButtonRelease(ex, ey, 1);
                m_lastPos = newPos;
                event->accept();
                update();
                return;
            }
        }
    }
}

void SceneWidget::keyReleaseEvent(QKeyEvent* event) {
    // 参考 osgQt-master：KeyRelease 忽略自动重复，仅在真实释放时触发
    if (event->isAutoRepeat()) { event->ignore(); return; }
    if (m_graphicsWindow.valid()) {
        if (auto* eq = m_graphicsWindow->getEventQueue()) {
            setKeyboardModifiers(m_graphicsWindow.get(), event);
            const int code = mapQtKeyToOsg(event->key());
            if (code != 0) {
                qDebug() << "[KeyRelease]" << event->key()
                         << " lastMouse(" << m_lastPos.x() << "," << m_lastPos.y() << ")";
                eq->keyRelease(code);
            }
        }
    }
}

// ========== 视口拾取到地理坐标 ==========
bool SceneWidget::computeGeoAt(const QPoint& pos, double& lon, double& lat, double& height) const {
    if (!m_view.valid()) {
        return false;
    }

    osgEarth::MapNode* mapNode = osgEarth::MapNode::findMapNode(m_view->getSceneData());
    if (!mapNode) {
        return false;
    }

    const double dpr = static_cast<double>(this->devicePixelRatioF());
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

void SceneWidget::initializeViewer() {
    if (m_viewerInitialized || !m_viewer.valid() || !m_view.valid()) {
        return;
    }

    m_view->setName("EmbeddedAirportView");
    m_view->setCameraManipulator(new osgEarth::Util::EarthManipulator());
    osgEarth::Util::MapNodeHelper().configureView(m_view.get());
    m_view->addEventHandler(new osgGA::StateSetManipulator(m_view->getCamera()->getOrCreateStateSet()));

    // 使用设备像素尺寸初始化嵌入式窗口与相机视口，避免高DPI下坐标与拖拽手感异常
    const float initDpr = static_cast<float>(this->devicePixelRatioF());
    const int initPixelW = static_cast<int>(width() * initDpr);
    const int initPixelH = static_cast<int>(height() * initDpr);
    m_graphicsWindow = new osgViewer::GraphicsWindowEmbedded(0, 0, initPixelW, initPixelH);
    osg::Camera* camera = m_view->getCamera();
    if (camera != nullptr) {
        camera->setGraphicsContext(m_graphicsWindow.get());
        camera->setViewport(0, 0, initPixelW, initPixelH);
        camera->setClearColor(osg::Vec4(0.1f, 0.1f, 0.15f, 1.0f));
    }
    // 初始化最近鼠标位置为视图中心，确保键盘事件初次使用时有稳定参考
    m_lastPos = QPoint(width() / 2, height() / 2);

    m_viewer->addView(m_view.get());
    applySceneData();
    updateCamera(std::max(1, width()), std::max(1, height()));
    // 初始化事件队列的指针位置为视图中心，避免首次方向键产生跳变
    if (m_graphicsWindow.valid()) {
        if (auto* eq = m_graphicsWindow->getEventQueue()) {
            const float dpr = static_cast<float>(this->devicePixelRatioF());
            const float mx = static_cast<float>(this->width() * 0.5f * dpr);
            const float my = static_cast<float>(this->height() * 0.5f * dpr);
            eq->mouseMotion(mx, my);
            m_lastPos = QPoint(width() / 2, height() / 2);
            qDebug() << "[Init] mouseMotion at center (" << m_lastPos.x() << "," << m_lastPos.y() << ")";
        }
    }
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
        m_lastAttachedSky = nullptr;
        return;
    }

    configureEnvironment();
}

void SceneWidget::updateCamera(int width, int height) const {
    if (!m_graphicsWindow.valid() || !m_view.valid()) {
        return;
    }

    const float dpr = static_cast<float>(this->devicePixelRatioF());
    const int pixelW = static_cast<int>(width * dpr);
    const int pixelH = static_cast<int>(height * dpr);

    m_graphicsWindow->resized(0, 0, pixelW, pixelH);
    if (auto* queue = m_graphicsWindow->getEventQueue()) {
        queue->windowResize(0, 0, pixelW, pixelH);
        // 同步事件队列的窗口矩形，确保键盘/鼠标事件的坐标归一化一致，避免首次键控跳变
        queue->syncWindowRectangleWithGraphicsContext();
    }

    if (osg::Camera* camera = m_view->getCamera()) {
        camera->setViewport(0, 0, pixelW, pixelH);
        const double aspect = static_cast<double>(pixelW) / static_cast<double>(pixelH);
        camera->setProjectionMatrixAsPerspective(30.0, aspect, kNearPlane, kFarPlane);
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

} // namespace earth::ui
