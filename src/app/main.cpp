#include "ui/MainWindow.h"

#include <QApplication>
#include <QResource>

/**
 * @brief Qt 应用入口，负责启动机场环境三维仿真主窗体。
 */
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    // 初始化资源文件
    Q_INIT_RESOURCE(ui_resources);
    
    earth::ui::MainWindow window;
    window.show();
    return app.exec();
}
