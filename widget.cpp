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
    : QOpenGLWidget(parent)
{
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

    for (int i=0; i<500; i++) {
        QString nodeA = createNode();
        QString nodeB = createNode();
        if (m_edges.value(nodeA) != nodeB && nodeGroups[nodeA] != nodeGroups[nodeB]) {
            m_edges.insert(nodeA, nodeB);
        }
    }

    setWindowFlags(Qt::Dialog);
    resize(1600, 1200);
    setMouseTracking(true);

    calculate();
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

    QPen nodePen;
    nodePen.setWidth(2);
    for (const QString &node : m_positions.keys()) {
        QColor nodeColor = m_nodeColors[node];
        nodeColor.setAlpha(128);
        nodePen.setColor(nodeColor);
        painter.setPen(nodePen);
        painter.drawPoint(m_positions[node]);
    }

    painter.setPen(Qt::NoPen);
    for (const Edge &edge : m_edgePaths) {
        if (edge.first == m_closest) {
            continue;
        }
        painter.setBrush(edge.brush);
        painter.drawPath(edge.path);
    }

    painter.setBrush(m_nodeColors[m_closest]);
    for (const Edge &edge : m_edgePaths) {
        if (edge.first != m_closest) {
            continue;
        }
        painter.drawPath(edge.path);
    }

    QColor penColor(Qt::white);
    painter.setBrush(m_nodeColors[m_closest]);
    painter.drawEllipse(m_positions[m_closest].x() - 5, m_positions[m_closest].y() - 5, 10, 10);
    painter.setPen(penColor);
    painter.drawText(m_positions[m_closest], m_closest);

    penColor.setAlpha(128);
    for (const QString &otherNode : m_edges.values(m_closest)) {
        QColor ellipseColor = m_nodeColors[otherNode];
        ellipseColor.setAlpha(128);
        painter.setPen(Qt::NoPen);
        painter.setBrush(ellipseColor);
        if (!m_positions.contains(otherNode)) {
            qWarning() << otherNode;
        }
        painter.drawEllipse(m_positions[otherNode].x() - 5, m_positions[otherNode].y() - 5, 10, 10);

        painter.setPen(penColor);
        painter.drawText(m_positions[otherNode], otherNode);
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    double minDist = width();
    QString closest;
    for (const QString node : m_positions.keys()) {
        double dist = hypot(event->x() - m_positions[node].x(), event->y() - m_positions[node].y());
        if (dist < 20 && dist < minDist) {
            minDist = dist;
            closest = node;
        }
    }
    if (closest != m_closest) {
        m_closest = closest;
        update();
    }
}

void Widget::resizeEvent(QResizeEvent *event)
{
    calculate();
    QOpenGLWidget::resizeEvent(event);
    update();
}

void Widget::calculate()
{
    QElapsedTimer timer;
    timer.start();
    m_positions.clear();
    m_nodeColors.clear();
    m_edgePaths.clear();

    int maxGroupSize = 0;
    const int numGroups = m_nodes.keys().count();
    const int hueStep = 359 / numGroups;
    int hue = 0;
    QMap<QString, QColor> groupColors;
    foreach(const QString group, m_nodes.keys()) {
        groupColors.insert(group, QColor::fromHsv(hue, 128, 255));
        hue += hueStep;

        maxGroupSize = qMax(maxGroupSize, m_nodes[group].count());
    }

    const int cx = width() / 2;
    const int cy = height() / 2;
    const double maxLength = qMin(width(), height()) - 20;
    const double radiusStep = (M_PI * 2) / m_nodes.keys().count();
    double r = 0;
    for(const QString &group : m_nodes.keys()) {
        const double axisLength = (m_nodes[group].count() / double(maxGroupSize)) * maxLength / 2;

        double offsetStep = (axisLength - 20) / m_nodes[group].count();
        double offset = 20;
        for (const QString node : m_nodes[group]) {
            int nodeX = cos(r) * offset + cx;
            int nodeY = sin(r) * offset + cy;
            m_positions.insert(node, QPoint(nodeX, nodeY));
            m_nodeColors.insert(node, groupColors[group]);

            offset += offsetStep;
        }
        r += radiusStep;
    }

    QPainterPathStroker stroker;
    stroker.setWidth(1.1);

    int lineAlpha = 192;
    if (m_edges.count() > 1000) {
        lineAlpha = 32;
    } else if (m_edges.count() > 100) {
        lineAlpha = 64;
    }
    for (QString node : m_positions.keys()) {
        if (!m_edges.contains(node)) {
            continue;
        }

        for (const QString &otherNode : m_edges.values(node)) {
            Edge edge;

            const double nodeX = m_positions[node].x();
            const double nodeY = m_positions[node].y();
            const double otherX = m_positions[otherNode].x();
            const double otherY = m_positions[otherNode].y();

            double magnitude = hypot(nodeX - cx, nodeY - cy);
            double otherMagnitude = hypot(otherX - cx, otherY - cy);
            double averageRadians = atan2(((nodeY - cy) + (otherY - cy))/2, ((nodeX - cx) + (otherX - cx))/2);

//            double averageMagnitude = (magnitude + otherMagnitude) / 2;
            double averageMagnitude = qMax(magnitude, otherMagnitude);
            QPointF controlPoint(cos(averageRadians) * averageMagnitude + cx, sin(averageRadians) * averageMagnitude + cy);

            QLinearGradient gradient(m_positions[node], m_positions[otherNode]);
            QColor color = m_nodeColors[node];
            color.setAlpha(lineAlpha);
            gradient.setColorAt(0, color);
            color.setAlpha(lineAlpha / 2);
            gradient.setColorAt(0.5, color);
            color = m_nodeColors[otherNode];
            color.setAlpha(lineAlpha / 3);
            gradient.setColorAt(1, color);
            edge.brush = QBrush(gradient);

            QPainterPath path;
            path.moveTo(m_positions[node]);
            path.quadTo(controlPoint, m_positions[otherNode]);
            edge.first = node;
            edge.second = otherNode;
            edge.path = stroker.createStroke(path);
            m_edgePaths.append(edge);
        }
    }
    qDebug() << "calculating took" << timer.restart() << "ms";
}
