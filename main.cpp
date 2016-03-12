#include "window.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName("qhiveplot");
    application.setOrganizationDomain("nous.ai");

    // Fix antialiasing quality of hive plot
    QSurfaceFormat fmt;
    fmt.setSamples(32);
    QSurfaceFormat::setDefaultFormat(fmt);

    Window window;
    window.show();

    return application.exec();
}
