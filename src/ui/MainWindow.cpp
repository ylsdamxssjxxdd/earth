#include "ui/MainWindow.h"

#include "core/SimulationBootstrapper.h"
#include "ui/SceneWidget.h"
#include "ui/draw/MapDrawingController.h"

#include <QAction>
#include <QActionGroup>
#include <QList>
#include <QStatusBar>
#include <QString>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>

#include <osgEarth/MapNode>
#include <osgDB/ReadFile>

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

    // 创建状态栏经纬度/高程显示，并连接SceneWidget鼠标地理位置信号
    if (!m_coordLabel) {
        m_coordLabel = new QLabel(this);
        m_coordLabel->setObjectName(QStringLiteral("coordLabel"));
        m_coordLabel->setMinimumWidth(320);
        m_coordLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_coordLabel->setText(tr("经度: ---, 纬度: ---, 高: ---"));
        if (auto* sb2 = statusBar()) {
            sb2->addPermanentWidget(m_coordLabel, 0);
        }
    }
    if (!m_fpsLabel) {
        m_fpsLabel = new QLabel(this);
        m_fpsLabel->setObjectName(QStringLiteral("fpsLabel"));
        m_fpsLabel->setMinimumWidth(120);
        m_fpsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_fpsLabel->setText(tr("帧率: -- FPS"));
        if (auto* sb2 = statusBar()) {
            sb2->addPermanentWidget(m_fpsLabel, 0);
        }
    }
    if (m_ui->openGLWidget) {
        connect(m_ui->openGLWidget, &SceneWidget::mouseGeoPositionChanged, this,
                [this](double lon, double lat, double height) {
                    if (!m_coordLabel) return;
                    m_coordLabel->setText(
                        tr("经度: %1°, 纬度: %2°, 高: %3 m")
                            .arg(QString::number(lon, 'f', 6))
                            .arg(QString::number(lat, 'f', 6))
                            .arg(QString::number(height, 'f', 1)));
                });
        connect(m_ui->openGLWidget, &SceneWidget::frameRateChanged, this,
                [this](double fps) {
                    if (!m_fpsLabel) return;
                    if (fps <= 0.0) {
                        m_fpsLabel->setText(tr("帧率: -- FPS"));
                    } else {
                        m_fpsLabel->setText(
                            tr("帧率: %1 FPS").arg(QString::number(fps, 'f', 1)));
                    }
                });
    }

    ensureDrawingController();
}

void MainWindow::registerActionHandlers() {
    // 为AddEarth动作添加特殊处理，连接到openEarthFile槽函数
    connect(m_ui->AddEarth, &QAction::triggered, this, &MainWindow::openEarthFile);
    
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
        m_ui->AddPolygon,
        m_ui->AddCircle,
        m_ui->FogEffect,
        m_ui->Rain,
        m_ui->Snow,
        m_ui->Cloud,
        m_ui->AddElevation,
        m_ui->VisibilityAnalysis,
        m_ui->ViewshedAnalysis,
        m_ui->RadarAnalysis,
        m_ui->WaterAnalysis,
        m_ui->TerrainProfileAnalysis,
        m_ui->Fire,
        m_ui->Distance,
        m_ui->Area,
        m_ui->Angle,
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

    setupDrawingActions();
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

void MainWindow::openEarthFile() {
    // 打开文件对话框，让用户选择.earth文件
    QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("打开Earth文件"),
        QString(),  // 默认目录
        tr("Earth文件 (*.earth);;所有文件 (*.*)")
    );
    
    if (filePath.isEmpty()) {
        return;  // 用户取消了选择
    }
    
    // 尝试加载.earth文件
    if (loadEarthFile(filePath)) {
        if (auto* sb = statusBar()) {
            sb->showMessage(tr("已成功加载Earth文件: %1").arg(filePath), 5000);
        }
    } else {
        QMessageBox::warning(
            this,
            tr("加载失败"),
            tr("无法加载Earth文件: %1").arg(filePath)
        );
    }
}

bool MainWindow::loadEarthFile(const QString& filePath) {
    if (!m_bootstrapper) {
        return false;
    }
    
    // 使用osgDB读取.earth文件
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(filePath.toStdString());
    if (!node) {
        return false;
    }
    
    // 尝试将节点转换为MapNode
    osgEarth::MapNode* mapNode = osgEarth::MapNode::findMapNode(node);
    if (!mapNode) {
        return false;
    }
    
    if (!m_bootstrapper->applyExternalScene(node.get())) {
        return false;
    }
    
    // 更新场景并把视图重置到Home参考点
    if (m_ui->openGLWidget) {
        m_ui->openGLWidget->setSimulation(m_bootstrapper.get());
        m_ui->openGLWidget->home();
        m_ui->openGLWidget->update();
    }

    ensureDrawingController();
    
    return true;
}

void MainWindow::setupDrawingActions() {
    if (!m_ui->AddPoint && !m_ui->AddLine && !m_ui->AddRectangle) {
        return;
    }

    if (m_drawingActionGroup == nullptr) {
        m_drawingActionGroup = new QActionGroup(this);
        m_drawingActionGroup->setExclusive(true);
    }

    const struct DrawingEntry {
        QAction* action = nullptr;
        draw::DrawingTool tool = draw::DrawingTool::None;
    } entries[] = {
        {m_ui->AddPoint, draw::DrawingTool::Point},
        {m_ui->AddLine, draw::DrawingTool::Polyline},
        {m_ui->AddRectangle, draw::DrawingTool::Rectangle},
    };

    for (const DrawingEntry& entry : entries) {
        if (!entry.action) {
            continue;
        }
        entry.action->setCheckable(true);
        if (!m_drawingActionGroup->actions().contains(entry.action)) {
            m_drawingActionGroup->addAction(entry.action);
        }
        connect(entry.action, &QAction::toggled, this, [this, tool = entry.tool](bool checked) {
            onDrawingActionToggled(tool, checked);
        });
    }

    if (m_ui->ClearAnalysis) {
        connect(m_ui->ClearAnalysis, &QAction::triggered, this, [this]() {
            if (m_drawingController) {
                m_drawingController->clearDrawings();
                m_drawingController->setTool(draw::DrawingTool::None);
            }
            if (m_drawingActionGroup) {
                for (QAction* action : m_drawingActionGroup->actions()) {
                    if (action->isChecked()) {
                        action->setChecked(false);
                    }
                }
            }
            if (auto* sb = statusBar()) {
                sb->showMessage(tr("已清空绘制结果"), 4000);
            }
        });
    }
}

void MainWindow::onDrawingActionToggled(draw::DrawingTool tool, bool checked) {
    if (!m_drawingController) {
        if (auto* sb = statusBar()) {
            sb->showMessage(tr("绘制控制器尚未初始化"), 4000);
        }
        return;
    }

    auto toolLabel = [tool]() -> QString {
        switch (tool) {
        case draw::DrawingTool::Point:
            return QObject::tr("点标绘");
        case draw::DrawingTool::Polyline:
            return QObject::tr("折线绘制");
        case draw::DrawingTool::Rectangle:
            return QObject::tr("矩形绘制");
        case draw::DrawingTool::None:
        default:
            return QObject::tr("绘制工具");
        }
    };

    if (checked) {
        m_drawingController->setTool(tool);
        if (auto* sb = statusBar()) {
            sb->showMessage(toolLabel() + tr(" 已启用，左键单击地图开始绘制。"), 5000);
        }
        return;
    }

    if (m_drawingActionGroup && m_drawingActionGroup->checkedAction() != nullptr) {
        return;
    }

    m_drawingController->setTool(draw::DrawingTool::None);
    if (auto* sb = statusBar()) {
        sb->showMessage(tr("已关闭绘制工具"), 3000);
    }
}

void MainWindow::ensureDrawingController() {
    if (!m_ui->openGLWidget) {
        return;
    }
    if (!m_drawingController) {
        m_drawingController = std::make_unique<draw::MapDrawingController>();
    }
    m_drawingController->attachSceneWidget(m_ui->openGLWidget);
    if (m_bootstrapper) {
        m_drawingController->setMapNode(m_bootstrapper->activeMapNode());
    }
}


} // namespace earth::ui
