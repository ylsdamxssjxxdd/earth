#include "ui/MainWindow.h"

#include <QApplication>

/**
 * @brief Qt 应用入口，负责启动机场环境三维仿真主窗体。
 */
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    earth::ui::MainWindow window;
    window.show();
    return app.exec();
}
