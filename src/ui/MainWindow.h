#pragma once

#include <QMainWindow>
#include <QString>
#include <memory>

#include "ui/draw/DrawingTypes.h"

class QAction;
class QActionGroup;
class QFileDialog;
class QLabel;

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
 * @brief 负责 Qt UI 与 osgEarth 场景桥接的主窗口。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief 完成 UI 绑定并初始化 osgEarth 渲染内容。
     */
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    /**
     * @brief “打开 .earth” 菜单动作。
     */
    void openEarthFile();
    /**
     * @brief 打开画笔样式配置，统一设置颜色与线宽。
     */
    void editDrawingStyle();

private:
    /**
     * @brief 构建嵌入式场景并刷新状态栏信息。
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
     * @brief 将 .earth 文件加载到当前场景。
     */
    bool loadEarthFile(const QString& filePath);

    /**
     * @brief 初始化菜单中的绘制动作，并关联状态提示。
     */
    void setupDrawingActions();

    /**
     * @brief 处理绘制工具的勾选切换，更新控制器与提示信息。
     */
    void onDrawingActionToggled(draw::DrawingTool tool, bool checked);

    /**
     * @brief 确保绘制控制器与 SceneWidget / MapNode 完成绑定。
     */
    void ensureDrawingController();

    /**
     * @brief 将当前画笔配置传递给绘制控制器。
     */
    void applyDrawingStyle();

    std::unique_ptr<Ui::EarthMainWindow> m_ui;
    std::unique_ptr<core::SimulationBootstrapper> m_bootstrapper;
    QLabel* m_coordLabel = nullptr;
    QLabel* m_fpsLabel = nullptr;
    QActionGroup* m_drawingActionGroup = nullptr;
    std::unique_ptr<draw::MapDrawingController> m_drawingController;
    draw::ColorRgba m_penColor {0.97F, 0.58F, 0.20F, 1.0F};
    double m_penThickness = 4.0;
};

} // namespace earth::ui
