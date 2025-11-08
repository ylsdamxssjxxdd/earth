#pragma once

#include <QMainWindow>
#include <memory>

namespace earth::core {
class SimulationBootstrapper;
}

namespace earth::ui {

class SceneWidget;

/**
 * @brief 应用主窗口，负责初始化Qt层UI并承载核心视图。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief 构造基础窗口并加载核心模块。
     */
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    /**
     * @brief 构建Qt控件层，配置菜单、工具栏与状态栏。
     */
    void setupUi();

    std::unique_ptr<core::SimulationBootstrapper> m_bootstrapper;
    SceneWidget* m_sceneWidget = nullptr;
};

} // namespace earth::ui
