#include "ui/MainWindow.h"

#include "core/SimulationBootstrapper.h"
#include "ui/SceneWidget.h"

#include <QDockWidget>
#include <QListWidget>
#include <QStatusBar>
#include <QToolBar>

namespace earth::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_bootstrapper(std::make_unique<core::SimulationBootstrapper>()) {
    m_bootstrapper->initialize();
    setupUi();
    m_sceneWidget->setSimulation(m_bootstrapper.get());
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setObjectName("AirportMainWindow");
    setWindowTitle(QStringLiteral("机场环境三维仿真原型"));
    resize(1280, 720);

    m_sceneWidget = new SceneWidget(this);
    setCentralWidget(m_sceneWidget);

    if (auto* status = statusBar()) {
        status->showMessage(QStringLiteral("系统初始化完成，正在准备场景…"));
    }

    auto* toolbar = addToolBar(QStringLiteral("查看"));
    toolbar->setMovable(false);
    toolbar->addAction(QStringLiteral("重置视角"), [this]() {
        if (m_bootstrapper) {
            m_sceneWidget->setSimulation(m_bootstrapper.get());
        }
    });

    auto* infoDock = new QDockWidget(QStringLiteral("运行概况"), this);
    infoDock->setObjectName("InfoDock");
    auto* infoList = new QListWidget(infoDock);
    infoList->addItem(QStringLiteral("场景：机场基础地形"));
    infoList->addItem(QStringLiteral("渲染：Qt5 + osgEarth/OSG"));
    infoList->addItem(QStringLiteral("计算：OpenCV 跑道掩码 %1x%2")
                          .arg(m_bootstrapper->runwayMask().cols)
                          .arg(m_bootstrapper->runwayMask().rows));
    infoDock->setWidget(infoList);
    addDockWidget(Qt::RightDockWidgetArea, infoDock);
}

} // namespace earth::ui
