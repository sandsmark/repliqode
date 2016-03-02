#include "widget.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QSurfaceFormat fmt;
    fmt.setSamples(2);
    QSurfaceFormat::setDefaultFormat(fmt);

    Widget w;
    w.show();

    return a.exec();
}
