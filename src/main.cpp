#include "mainwindow.h"

#include <QApplication>
#include <QFileInfo>
#include <QStringList>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("Lightweight E-Reader");
    QCoreApplication::setOrganizationName("YourName");

    MainWindow window;
    window.show();

    const QStringList arguments = QCoreApplication::arguments();

    if (arguments.size() > 1) {
        const QString filePath =
            QFileInfo(arguments.at(1)).absoluteFilePath();

        window.openFileFromPath(filePath);
    }

    return app.exec();
}