#include "window.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName("qhiveplot");
    application.setOrganizationDomain("nous.ai");

    // Invert the palette, for reasons
    QPalette palette = QApplication::palette();

    for (int group = 0; group < QPalette::NColorGroups; group++) {
        QPalette::ColorGroup colorGroup = QPalette::ColorGroup(group);
        for (int role = 0; role < QPalette::NColorRoles; role++) {
            QPalette::ColorRole colorRole = QPalette::ColorRole(role);
            QColor color = palette.color(colorGroup, colorRole);
            QColor inverted(255 - color.red(), 255 - color.green(), 255 - color.blue(), color.alpha());
            palette.setColor(colorGroup, colorRole, inverted);
        }
    }
    QApplication::setPalette(palette);

    // Fix antialiasing quality of hive plot
    QSurfaceFormat fmt;
    fmt.setSamples(32);
    QSurfaceFormat::setDefaultFormat(fmt);

    Window window;
    window.showFullScreen();

    return application.exec();
}
