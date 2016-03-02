#include "widget.h"
#include <QDebug>
#include <qmath.h>
#include <QPainter>
#include <QElapsedTimer>
#include <QMouseEvent>

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
        num = qrand() % 100;
        break;
    default:
        type = "reduction %1";
        num = qrand() % 50;
        break;
    }
    return type.arg(num);
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    qsrand(4);
    setWindowFlags(Qt::Dialog);
    for(int i = 0; i<75; i++) {
        m_nodes["markers"].append(QString("marker %1").arg(i));
    }
    for(int i = 0; i<100; i++) {
        m_nodes["programs"].append(QString("program %1").arg(i));
    }
    for(int i = 0; i<50; i++) {
        m_nodes["reductions"].append(QString("reduction %1").arg(i));
    }

    QMap<QString, QString> nodeGroups;
    for (const QString &group : m_nodes.keys()) {
        for (const QString &node : m_nodes.value(group)) {
            nodeGroups[node] = group;
        }
    }
    for (int i=0; i<1500; i++) {
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
    setMouseTracking(true);
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

    painter.setPen(Qt::white);
    painter.drawText(10, 20, m_closest);

    const int cx = width() / 2;
    const int cy = height() / 2;

    const int numGroups = m_nodes.keys().count();
    const double radiusStep = (M_PI * 2) / numGroups;
    double r = 0;
    const double maxLength = qMin(width(), height()) - 20;
    QMap<QString, QColor> nodeColors;
    m_positions.clear();
    for(const QString group : m_nodes.keys()) {
        const double axisLength = maxLength / 2;//(m_nodes[group].count() / double(m_maxNodes)) * maxLength / 2;
        int startX = cos(r) * 20 + cx;
        int startY = sin(r) * 20 + cy;
        int endX = cos(r) * axisLength + cx;
        int endY = sin(r) * axisLength + cy;
        painter.setPen(QPen(m_colors[group], 0.9));
        painter.drawLine(startX, startY, endX, endY);

        painter.drawText(cos(r) * (axisLength + 20) + cx, sin(r) * (axisLength + 20) + cy, group);

        double offsetStep = (axisLength - 20) / m_nodes[group].count();
        double offset = 20;
        for (const QString node : m_nodes[group]) {
            int nodeX = cos(r) * offset + cx;
            int nodeY = sin(r) * offset + cy;
            if (node == m_closest) {
                painter.setBrush(m_colors[group]);
                painter.drawEllipse(nodeX - 5 , nodeY - 5, 10, 10);
            } else {
                painter.drawEllipse(nodeX - 1 , nodeY - 1, 2, 2);
            }
            m_positions.insert(node, QPoint(nodeX, nodeY));
            nodeColors.insert(node, m_colors[group]);

            offset += offsetStep;
        }


        r += radiusStep;
    }
    qDebug() << timer.restart() << "ms for drawing nodes";

    painter.setPen(Qt::NoPen);
    QPainterPathStroker stroker;
    stroker.setWidth(0.5);
    for (QString node : m_positions.keys()) {
        if (!m_edges.contains(node)) {
            continue;
        }
        for (const QString &otherNode : m_edges.values(node)) {
            const double nodeX = m_positions[node].x();
            const double nodeY = m_positions[node].y();
            const double otherX = m_positions[otherNode].x();
            const double otherY = m_positions[otherNode].y();

            double magnitude = hypot(nodeX - cx, nodeY - cy);
            double otherMagnitude = hypot(otherX - cx, otherY - cy);
            double averageRadians = atan2(((nodeY - cy) + (otherY - cy))/2, ((nodeX - cx) + (otherX - cx))/2);

//            double averageMagnitude = qMax(magnitude, otherMagnitude);
            double averageMagnitude = (magnitude + otherMagnitude) / 2;
            QPointF controlPoint(cos(averageRadians) * averageMagnitude + cx, sin(averageRadians) * averageMagnitude + cy);

            QLinearGradient gradient(m_positions[node], m_positions[otherNode]);
            gradient.setColorAt(0, nodeColors[node]);
            gradient.setColorAt(1, nodeColors[otherNode]);

            QBrush brush(gradient);
            painter.setBrush(brush);

            if (node == m_closest || otherNode == m_closest) {
                stroker.setWidth(2);
            } else {
                stroker.setWidth(0.5);
            }

            QPainterPath path;
            path.moveTo(m_positions[node]);
            path.quadTo(controlPoint, m_positions[otherNode]);
            painter.drawPath(stroker.createStroke(path));
        }
    }
    qDebug() << timer.restart() << "ms for drawing edges";
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    double minDist = width();
    QString closest;
    for (const QString node : m_positions.keys()) {
        double dist = hypot(event->x() - m_positions[node].x(), event->y() - m_positions[node].y());
        if (dist < minDist) {
            minDist = dist;
            closest = node;
        }
    }
    if (closest != m_closest) {
        m_closest = closest;
        update();
    }
}
