#pragma once

#include <QMainWindow>
#include <memory>
#include <QString>

class QAction;
class QFileDialog;
class QLabel;

namespace Ui {
class EarthMainWindow;
}

namespace earth::core {
class SimulationBootstrapper;
}

namespace earth::ui {

/**
 * @brief 复用Qt Designer生成的UI并与仿真骨架对接的主窗口。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief 完成UI绑定并初始化osgEarth渲染内容。
     */
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    /**
     * @brief 处理打开.earth文件的菜单动作
     */
    void openEarthFile();

private:
    /**
     * @brief 构建仿真场景并更新状态栏提示信息。
     */
    void initializeSimulation();

    /**
     * @brief 为菜单和工具条动作注册统一的骨架回调。
     */
    void registerActionHandlers();

    /**
     * @brief 绑定指定动作，输出占位日志供后续完善。
     */
    void bindAction(QAction* action);

    /**
     * @brief 统一处理动作触发事件并展示当前状态。
     */
    void handleActionTriggered(QAction* action, bool checked);

    /**
     * @brief 加载指定的.earth文件到场景中
     * @param filePath .earth文件的路径
     * @return 是否加载成功
     */
    bool loadEarthFile(const QString& filePath);

    std::unique_ptr<Ui::EarthMainWindow> m_ui;
    std::unique_ptr<core::SimulationBootstrapper> m_bootstrapper;
    QLabel* m_coordLabel = nullptr;
    QLabel* m_fpsLabel = nullptr;
};

} // namespace earth::ui
