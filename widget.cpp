#include "widget.h"
#include <QDebug>
#include <qmath.h>
#include <QPainter>
#include <QElapsedTimer>

QString createNode()
{
    QString type;
    int r = qrand() % 3;
    int num = 0;
    switch (r) {
    case 0:
        type = "marker %1";
        num = qrand() % 100;
        break;
    case 1:
        type = "program %1";
        num = qrand() % 150;
        break;
    default:
        type = "reduction %1";
        num = qrand() % 150;
        break;
    }
    return type.arg(num);
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    qsrand(4);
    setWindowFlags(Qt::Dialog);
    for(int i = 0; i<100; i++) {
        m_nodes["markers"].append(QString("marker %1").arg(i));
    }
    for(int i = 0; i<150; i++) {
        m_nodes["programs"].append(QString("program %1").arg(i));
    }
    for(int i = 0; i<150; i++) {
        m_nodes["reductions"].append(QString("reduction %1").arg(i));
    }

    QMap<QString, QString> nodeGroups;
    for (const QString &group : m_nodes.keys()) {
        for (const QString &node : m_nodes.value(group)) {
            nodeGroups[node] = group;
        }
    }
    for (int i=0; i<500; i++) {
        QString nodeA = createNode();
        QString nodeB = createNode();
        if (m_edges.value(nodeA) != nodeB && nodeGroups[nodeA] != nodeGroups[nodeB]) {
            m_edges.insert(nodeA, nodeB);
        }
    }

    m_maxNodes = 0;
    const int numGroups = m_nodes.keys().count();
    const int hueStep = 359 / numGroups;
    int hue = 0;
    foreach(const QString group, m_nodes.keys()) {
        m_colors.insert(group, QColor::fromHsv(hue, 255, 230));
        hue += hueStep;

        m_maxNodes = qMax(m_maxNodes, m_nodes[group].count());
    }
    qDebug() << m_maxNodes;
    resize(1200, 1200);
}

Widget::~Widget()
{

}

void Widget::paintEvent(QPaintEvent *)
{
    QElapsedTimer timer;
    timer.start();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), Qt::black);
    const int cx = width() / 2;
    const int cy = height() / 2;
//    painter.setPen(QPen(Qt::white, 0.5));
//    painter.drawEllipse(cx - 20, cy - 20, 40, 40);

//    painter.setPen(Qt::NoPen);
    const int numGroups = m_nodes.keys().count();
    const double radiusStep = (M_PI * 2) / numGroups;
    double r = 0;
    const double maxLength = qMin(width(), height()) - 20;
    QMap<QString, QPoint> nodePositions;
    QMap<QString, QColor> nodeColors;
    for(const QString group : m_nodes.keys()) {
        const double axisLength = (m_nodes[group].count() / double(m_maxNodes)) * maxLength / 2;
//        painter.setBrush((m_colors[group]));
        int startX = cos(r) * 20 + cx;
        int startY = sin(r) * 20 + cy;
        int endX = cos(r) * axisLength + cx;
        int endY = sin(r) * axisLength + cy;
        painter.setPen(QPen(m_colors[group], 2));
        painter.drawLine(startX, startY, endX, endY);

//        painter.save();
//        painter.translate(endX, endY);
//        painter.rotate(360 * r / (M_PI * 2));
        painter.drawText(cos(r) * (axisLength + 20) + cx, sin(r) * (axisLength + 20) + cy, group);
//        painter.restore();


        double offsetStep = (axisLength - 20) / m_nodes[group].count();
        double offset = 20;
        for (const QString node : m_nodes[group]) {
            int nodeX = cos(r) * offset + cx;
            int nodeY = sin(r) * offset + cy;
//            painter.drawEllipse(nodeX - 2, nodeY - 2, 4, 4);
            nodePositions.insert(node, QPoint(nodeX, nodeY));
            nodeColors.insert(node, m_colors[group]);

            offset += offsetStep;
        }


        r += radiusStep;
    }
    qDebug() << timer.restart() << "ms for drawing nodes";

    painter.setPen(Qt::NoPen);
    QPainterPathStroker stroker;
    stroker.setWidth(0.5);
    for (QString node : nodePositions.keys()) {
        if (!m_edges.contains(node)) {
            continue;
        }
        for (const QString &otherNode : m_edges.values(node)) {
            const double nodeX = nodePositions[node].x();
            const double nodeY = nodePositions[node].y();
            const double otherX = nodePositions[otherNode].x();
            const double otherY = nodePositions[otherNode].y();

            double magnitude = hypot(nodeX - cx, nodeY - cy);
            double otherMagnitude = hypot(otherX - cx, otherY - cy);
            double averageRadians = atan2(((nodeY - cy) + (otherY - cy))/2, ((nodeX - cx) + (otherX - cx))/2);

//            double averageMagnitude = qMax(magnitude, otherMagnitude);//(magnitude + otherMagnitude) / 2;
            double averageMagnitude = (magnitude + otherMagnitude) / 2;
            QPointF controlPoint(cos(averageRadians) * averageMagnitude + cx, sin(averageRadians) * averageMagnitude + cy);

            QLinearGradient gradient(nodePositions[node], nodePositions[otherNode]);
            gradient.setColorAt(0, nodeColors[node]);
            gradient.setColorAt(1, nodeColors[otherNode]);

            QBrush brush(gradient);
            painter.setBrush(brush);

            QPainterPath path;
            path.moveTo(nodePositions[node]);
            path.quadTo(controlPoint, nodePositions[otherNode]);
            painter.drawPath(stroker.createStroke(path));
        }
    }
    qDebug() << timer.restart() << "ms for drawing edges";
}
