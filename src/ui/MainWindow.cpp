#include "ui/MainWindow.h"

#include "core/SimulationBootstrapper.h"
#include "ui/SceneWidget.h"

#include <QAction>
#include <QList>
#include <QStatusBar>
#include <QString>

#include "ui_MainWindow.h"

namespace earth::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ui(std::make_unique<Ui::EarthMainWindow>())
    , m_bootstrapper(std::make_unique<core::SimulationBootstrapper>()) {
    m_ui->setupUi(this);
    initializeSimulation();
    registerActionHandlers();
}

MainWindow::~MainWindow() = default;

void MainWindow::initializeSimulation() {
    if (!m_bootstrapper) {
        return;
    }

    m_bootstrapper->initialize();

    if (m_ui->openGLWidget != nullptr) {
        m_ui->openGLWidget->setSimulation(m_bootstrapper.get());
    }

    if (auto* sb = statusBar()) {
        sb->showMessage(tr("场景骨架已装载，可通过菜单触发各项功能。"));
    }
}

void MainWindow::registerActionHandlers() {
    const QList<QAction*> actions = {
        m_ui->SetLosHeight,
        m_ui->ViewshedPara,
        m_ui->Fuzhou,
        m_ui->Boston,
        m_ui->Airport,
        m_ui->SciencePark,
        m_ui->information,
        m_ui->DongBaoShan,
        m_ui->AddMiniMap,
        m_ui->AddScaleBar,
        m_ui->AddCompass,
        m_ui->ProvincialBoundary,
        m_ui->FeaturesQuery,
        m_ui->AddCityModel,
        m_ui->PathRoaming,
        m_ui->AddTiltphotographymodel,
        m_ui->Addgraticule,
        m_ui->AddKml,
        m_ui->AddVector,
        m_ui->AddPoint,
        m_ui->AddLine,
        m_ui->AddRectangle,
        m_ui->AddPolygon,
        m_ui->AddCircle,
        m_ui->FogEffect,
        m_ui->Rain,
        m_ui->Snow,
        m_ui->Cloud,
        m_ui->AddElevation,
        m_ui->AddEarth,
        m_ui->VisibilityAnalysis,
        m_ui->ViewshedAnalysis,
        m_ui->RadarAnalysis,
        m_ui->WaterAnalysis,
        m_ui->TerrainProfileAnalysis,
        m_ui->Fire,
        m_ui->Distance,
        m_ui->Area,
        m_ui->Angle,
        m_ui->ClearAnalysis,
        m_ui->AddModel,
        m_ui->Addsatellite,
        m_ui->Tianwa,
        m_ui->Dynamictexture,
        m_ui->TrailLine,
        m_ui->slopeAnalysis,
        m_ui->StraightArrow,
        m_ui->DoubleArrow,
        m_ui->DiagonalArrow,
        m_ui->Lune,
        m_ui->GatheringPlace,
        m_ui->ParallelSearch,
        m_ui->SectorSearch,
        m_ui->ChangeTime,
    };

    for (QAction* action : actions) {
        bindAction(action);
    }
}

void MainWindow::bindAction(QAction* action) {
    if (action == nullptr) {
        return;
    }

    connect(action, &QAction::triggered, this, [this, action](bool checked) {
        handleActionTriggered(action, checked);
    });
}

void MainWindow::handleActionTriggered(QAction* action, bool checked) {
    if (action == nullptr) {
        return;
    }

    const QString name = !action->text().isEmpty() ? action->text() : action->objectName();

    if (auto* sb = statusBar()) {
        sb->showMessage(
            tr("%1 功能骨架尚未接入，实现中……（状态：%2）")
                .arg(name)
                .arg(checked ? tr("开启") : tr("关闭")),
            4000);
    }
}

} // namespace earth::ui
