#include "ui/MainWindow.h"

#include "core/SimulationBootstrapper.h"
#include "ui/SceneWidget.h"

#include <QAction>
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
    }
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
        m_ui->AddRectangle,
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
    
    return true;
}


} // namespace earth::ui
