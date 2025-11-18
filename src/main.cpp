#include <QApplication>
#include "main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(1280, 960);
    w.show();
    return app.exec();
}
