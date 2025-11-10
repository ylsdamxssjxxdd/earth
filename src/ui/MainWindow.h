#pragma once

#include <QMainWindow>
#include <memory>
#include <QString>

#include "ui/draw/DrawingTypes.h"

class QAction;
class QFileDialog;
class QLabel;
class QActionGroup;

namespace Ui {
class EarthMainWindow;
}

namespace earth::core {
class SimulationBootstrapper;
}

namespace earth::ui::draw {
class MapDrawingController;
}

namespace earth::ui {

/**
 * @brief 复用Qt Designer生成的UI并与仿真骨架对接的主窗口。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief 负责 UI 绑定与 osgEarth 渲染初始化的主窗口。
     */
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    /**
     * @brief “打开.earth”菜单动作回调。
     */
    void openEarthFile();

private:
    /**
     * @brief 构建嵌入式场景并更新状态栏信息。
     */
    void initializeSimulation();

    /**
     * @brief 为菜单/工具条动作注册统一的信号槽。
     */
    void registerActionHandlers();

    /**
     * @brief 绑定指定动作并输出占位日志，便于后续扩展。
     */
    void bindAction(QAction* action);

    /**
     * @brief 统一处理未实现动作的触发事件并展示当前状态。
     */
    void handleActionTriggered(QAction* action, bool checked);

    /**
     * @brief 加载 .earth 文件到场景中。
     */
    bool loadEarthFile(const QString& filePath);

    /**
     * @brief 初始化菜单中的绘制动作，配置互斥选择与状态提示。
     */
    void setupDrawingActions();

    /**
     * @brief 处理绘制工具的勾选切换，及时更新绘制控制器与状态栏提示。
     */
    void onDrawingActionToggled(draw::DrawingTool tool, bool checked);

    /**
     * @brief 确保绘制控制器与 SceneWidget、MapNode 完成绑定。
     */
    void ensureDrawingController();

    std::unique_ptr<Ui::EarthMainWindow> m_ui;
    std::unique_ptr<core::SimulationBootstrapper> m_bootstrapper;
    QLabel* m_coordLabel = nullptr;
    QLabel* m_fpsLabel = nullptr;
    QActionGroup* m_drawingActionGroup = nullptr;
    std::unique_ptr<draw::MapDrawingController> m_drawingController;
};

} // namespace earth::ui



