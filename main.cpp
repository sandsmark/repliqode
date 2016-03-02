#include "hivewidget.h"
#include <QApplication>
#include <QSurfaceFormat>

// For generating test data
QString createNode()
{
    QString type;
    int r = qrand() % 3;
    int num = 0;
    switch (r) {
    case 0:
        type = "marker %1";
        num = qrand() % 75;
        break;
    case 1:
        type = "program %1";
        num = qrand() % 50;
        break;
    default:
        type = "reduction %1";
        num = qrand() % 50;
        break;
    }
    return type.arg(num);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QSurfaceFormat fmt;
    fmt.setSamples(2);
    QSurfaceFormat::setDefaultFormat(fmt);

    // Generate some test data to work on
    QMultiMap<QString, QString> nodes;
    for(int i = 0; i<75; i++) {
        nodes.insert("markers", QString("marker %1").arg(i));
    }
    for(int i = 0; i<50; i++) {
        nodes.insert("programs", QString("program %1").arg(i));
    }
    for(int i = 0; i<50; i++) {
        nodes.insert("reductions", QString("reduction %1").arg(i));
    }

    QMap<QString, QString> nodeGroups;
    for (const QString &group : nodes.keys()) {
        for (const QString &node : nodes.values(group)) {
            nodeGroups[node] = group;
        }
    }

    QList<Edge> edges;
    for (int i=0; i<50; i++) {
        QString nodeA = createNode();
        QString nodeB = createNode();
        Edge edge;
        edge.source = nodeA;
        edge.target = nodeB;
        if (!edges.contains(edge)) {
            edges.append(edge);
        }
    }

    HiveWidget hiveWidget;
    hiveWidget.setNodes(nodes);
    hiveWidget.setEdges(edges);
    hiveWidget.setWindowFlags(Qt::Dialog);
    hiveWidget.resize(1600, 1200);
    hiveWidget.show();

    return a.exec();
}
