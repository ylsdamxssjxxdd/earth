#include <QTimer>
#include <QApplication>
#include <QGridLayout>

#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgGA/TrackballManipulator>

#include <osgDB/ReadFile>

#include <osgQt/GraphicsWindowQt>
#include <osg/State>

#include <iostream>

namespace
{
    // Trackball manipulator that also reacts to arrow keys for rotation/pan/zoom.
    class KeyboardTrackballManipulator : public osgGA::TrackballManipulator
    {
    public:
        KeyboardTrackballManipulator()
        : osgGA::TrackballManipulator()
        , _rotationStep(0.03f)    // normalized screen delta per key press
        , _panRatio(0.05f)        // fraction of distance per key press
        , _zoomRatio(0.1f)        // fraction of distance per key press
        {}

    protected:
        bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override
        {
            if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
            {
                if (handleKeyboardInput(ea))
                {
                    us.requestRedraw();
                    us.requestContinuousUpdate(false);
                    return true;
                }
            }

            return osgGA::TrackballManipulator::handle(ea, us);
        }

    private:
        bool handleKeyboardInput(const osgGA::GUIEventAdapter& ea)
        {
            const float panStep = _panRatio * static_cast<float>(getDistance());
            const unsigned int mods = ea.getModKeyMask();

            switch (ea.getKey())
            {
                case osgGA::GUIEventAdapter::KEY_Left:
                    if (mods & osgGA::GUIEventAdapter::MODKEY_CTRL)
                        panModel(-panStep, 0.f);
                    else
                        rotateWithFixedVertical(-_rotationStep, 0.f);
                    return true;
                case osgGA::GUIEventAdapter::KEY_Right:
                    if (mods & osgGA::GUIEventAdapter::MODKEY_CTRL)
                        panModel(panStep, 0.f);
                    else
                        rotateWithFixedVertical(_rotationStep, 0.f);
                    return true;
                case osgGA::GUIEventAdapter::KEY_Up:
                    if (mods & osgGA::GUIEventAdapter::MODKEY_SHIFT)
                        zoomModel(-_zoomRatio, true);
                    else if (mods & osgGA::GUIEventAdapter::MODKEY_CTRL)
                        panModel(0.f, panStep);
                    else
                        rotateWithFixedVertical(0.f, _rotationStep);
                    return true;
                case osgGA::GUIEventAdapter::KEY_Down:
                    if (mods & osgGA::GUIEventAdapter::MODKEY_SHIFT)
                        zoomModel(_zoomRatio, true);
                    else if (mods & osgGA::GUIEventAdapter::MODKEY_CTRL)
                        panModel(0.f, -panStep);
                    else
                        rotateWithFixedVertical(0.f, -_rotationStep);
                    return true;
                default:
                    break;
            }

            return false;
        }

        float _rotationStep;
        float _panRatio;
        float _zoomRatio;
    };
}

class ViewerWidget : public QWidget, public osgViewer::CompositeViewer
{
public:
    ViewerWidget(QWidget* parent = 0, Qt::WindowFlags f = 0, osgViewer::ViewerBase::ThreadingModel threadingModel=osgViewer::CompositeViewer::SingleThreaded) : QWidget(parent, f)
    {
        setThreadingModel(threadingModel);

        // disable the default setting of viewer.done() by pressing Escape.
        setKeyEventSetsDone(0);

        QWidget* widget1 = addViewWidget( createGraphicsWindow(0,0,100,100), osgDB::readRefNodeFile("cow.osgt") );
        QWidget* widget2 = addViewWidget( createGraphicsWindow(0,0,100,100), osgDB::readRefNodeFile("j20_high.osgb") );
        QWidget* widget3 = addViewWidget( createGraphicsWindow(0,0,100,100), osgDB::readRefNodeFile("axes.osgt") );
        QWidget* widget4 = addViewWidget( createGraphicsWindow(0,0,100,100), osgDB::readRefNodeFile("fountain.osgt") );

        QGridLayout* grid = new QGridLayout;
        grid->addWidget( widget1, 0, 0 );
        grid->addWidget( widget2, 0, 1 );
        grid->addWidget( widget3, 1, 0 );
        grid->addWidget( widget4, 1, 1 );
        setLayout( grid );
        widget1->setFocus(Qt::ActiveWindowFocusReason);

        connect( &_timer, SIGNAL(timeout()), this, SLOT(update()) );
        _timer.start( 10 );
    }

    QWidget* addViewWidget( osgQt::GraphicsWindowQt* gw, osg::ref_ptr<osg::Node> scene )
    {
        osgViewer::View* view = new osgViewer::View;
        addView( view );

        osg::Camera* camera = view->getCamera();
        camera->setGraphicsContext( gw );

        const osg::GraphicsContext::Traits* traits = gw->getTraits();

        camera->setClearColor( osg::Vec4(0.2, 0.2, 0.6, 1.0) );
        camera->setViewport( new osg::Viewport(0, 0, traits->width, traits->height) );
        camera->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(traits->width)/static_cast<double>(traits->height), 1.0f, 10000.0f );

        view->setSceneData( scene );
        view->addEventHandler( new osgViewer::StatsHandler );
        view->setCameraManipulator( new KeyboardTrackballManipulator );
        gw->setTouchEventsEnabled( false );
        return gw->getGLWidget();
    }

    osgQt::GraphicsWindowQt* createGraphicsWindow( int x, int y, int w, int h, const std::string& name="", bool windowDecoration=false )
    {
        osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
        traits->windowName = name;
        traits->windowDecoration = windowDecoration;
        traits->x = x;
        traits->y = y;
        traits->width = w;
        traits->height = h;
        traits->doubleBuffer = true;
        traits->alpha = ds->getMinimumNumAlphaBits();
        traits->stencil = ds->getMinimumNumStencilBits();
        traits->sampleBuffers = ds->getMultiSamples();
        traits->samples = ds->getNumMultiSamples();
        return new osgQt::GraphicsWindowQt(traits.get());
    }

    virtual void paintEvent( QPaintEvent* /*event*/ )
    { frame(); }

protected:

    QTimer _timer;
};

int main( int argc, char** argv )
{
    osg::ArgumentParser arguments(&argc, argv);
    osg::DisplaySettings::instance()->setGLContextVersion("2.1");
    osg::DisplaySettings::instance()->setGLContextProfileMask(0);
    osg::DisplaySettings::instance()->setGLContextFlags(0);

#if QT_VERSION >= 0x050000
    // Qt5 is currently crashing and reporting "Cannot make QOpenGLContext current in a different thread" when the viewer is run multi-threaded, this is regression from Qt4
    osgViewer::ViewerBase::ThreadingModel threadingModel = osgViewer::ViewerBase::SingleThreaded;
#else
    osgViewer::ViewerBase::ThreadingModel threadingModel = osgViewer::ViewerBase::CullDrawThreadPerContext;
#endif

    while (arguments.read("--SingleThreaded")) threadingModel = osgViewer::ViewerBase::SingleThreaded;
    while (arguments.read("--CullDrawThreadPerContext")) threadingModel = osgViewer::ViewerBase::CullDrawThreadPerContext;
    while (arguments.read("--DrawThreadPerContext")) threadingModel = osgViewer::ViewerBase::DrawThreadPerContext;
    while (arguments.read("--CullThreadPerCameraDrawThreadPerContext")) threadingModel = osgViewer::ViewerBase::CullThreadPerCameraDrawThreadPerContext;

#if QT_VERSION >= 0x040800
    // Required for multithreaded QGLWidget on Linux/X11, see http://blog.qt.io/blog/2011/06/03/threaded-opengl-in-4-8/
    if (threadingModel != osgViewer::ViewerBase::SingleThreaded)
        QApplication::setAttribute(Qt::AA_X11InitThreads);
#endif

    QApplication app(argc, argv);
    ViewerWidget* viewWidget = new ViewerWidget(0, Qt::Widget, threadingModel);
    viewWidget->setGeometry( 100, 100, 800, 600 );
    viewWidget->show();
    return app.exec();
}
