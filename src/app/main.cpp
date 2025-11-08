#include "ui/MainWindow.h"
#include "core/EnvironmentBootstrapper.h"

#include <QApplication>
#include <QCoreApplication>
#include <QResource>

/**
 * @brief Qt 应用程序入口，负责初始化环境并启动三维仿真主窗体。
 */
int main(int argc, char* argv[]) {
    QCoreApplication::setOrganizationName(QStringLiteral("EarthSimLab"));
    QCoreApplication::setApplicationName(QStringLiteral("airport-earth"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QApplication app(argc, argv);

    // 初始化资源文件
    Q_INIT_RESOURCE(ui_resources);
    earth::core::EnvironmentBootstrapper::instance().initialize();

    earth::ui::MainWindow window;
    window.show();
    return app.exec();
}
