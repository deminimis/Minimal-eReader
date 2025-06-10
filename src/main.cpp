#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Lightweight E-Reader");
    QCoreApplication::setOrganizationName("YourName");
    MainWindow window;
    window.show();
    return app.exec();
}
