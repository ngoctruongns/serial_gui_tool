#include <QApplication>
#include "main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(700, 500);
    w.show();
    return app.exec();
}
