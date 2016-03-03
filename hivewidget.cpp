#include "hivewidget.h"
#include <QDebug>
#include <qmath.h>
#include <QPainter>
#include <QElapsedTimer>
#include <QMouseEvent>

HiveWidget::HiveWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_scaleEdgeMax(true)
{
    setMouseTracking(true);
}

HiveWidget::~HiveWidget()
{

}

void HiveWidget::setNodes(const QMultiMap<QString, QString> &nodes)
{
    m_nodes = nodes;
    calculate();
    update();
}

void HiveWidget::setEdges(const QList<Edge> &edges)
{
    m_edges = edges;
    calculate();
    update();
}

void HiveWidget::paintEvent(QPaintEvent *)
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

    // Draw underlying edges first
    painter.setPen(Qt::NoPen);

    for (const Edge &edge : m_edges) {
        if (edge.source == m_closest) {
            continue;
        }
        painter.setBrush(edge.brush);
        painter.drawPath(edge.path);
    }

    // Draw active edges on top
    painter.setBrush(m_nodeColors[m_closest]);
    for (const Edge &edge : m_edges) {
        if (edge.source != m_closest) {
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
    for (const Edge &edge : m_edges) {
        if (edge.source != m_closest) {
            continue;
        }

        QColor ellipseColor = m_nodeColors[edge.target];
        ellipseColor.setAlpha(128);
        painter.setPen(Qt::NoPen);
        painter.setBrush(ellipseColor);
        painter.drawEllipse(m_positions[edge.target].x() - 5, m_positions[edge.target].y() - 5, 10, 10);

        painter.setPen(penColor);
        painter.drawText(m_positions[edge.target], edge.target);
    }
}

void HiveWidget::mouseMoveEvent(QMouseEvent *event)
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

void HiveWidget::resizeEvent(QResizeEvent *event)
{
    calculate();
    QOpenGLWidget::resizeEvent(event);
    update();
}

void HiveWidget::calculate()
{
    QElapsedTimer timer;
    timer.start();
    m_positions.clear();
    m_nodeColors.clear();

    if (m_nodes.isEmpty() || m_edges.isEmpty()) {
        return;
    }

    int maxGroupSize = 0;
    const int numGroups = m_nodes.uniqueKeys().count();

    // Automatically generate some colors
    const int hueStep = 359 / numGroups;
    int hue = 0;
    QMap<QString, QColor> groupColors;
    foreach(const QString group, m_nodes.uniqueKeys()) {
        groupColors.insert(group, QColor::fromHsv(hue, 128, 255));
        hue += hueStep;

        maxGroupSize = qMax(maxGroupSize, m_nodes.count(group));
    }


    QHash<QString, double> nodeAngles;
    const int cx = width() / 2;
    const int cy = height() / 2;
    const double angleStep = (M_PI * 2) / numGroups;
    double angle = 0;
    double maxLength = (qMax(width(), height())) / 2;

    // Find the optimal scale
    for (int i=0; i<numGroups; i++) {
        if (angle > M_PI) {
            angle -= M_PI;
        }
        if (angle < M_PI / 2) {
            maxLength = qMin(maxLength, cx / cos(angle));
        } else {
            maxLength = qMin(maxLength, cy / cos(angle - M_PI));
        }
        angle += angleStep;
    }
    maxLength -= 20;
    angle = 0;
    for(const QString &group : m_nodes.uniqueKeys()) {
        const double axisLength = (m_nodes.count(group) / double(maxGroupSize)) * maxLength - 20;

        double offsetStep = (axisLength - 20) / m_nodes.count(group);
        double offset = 20;
        for (const QString node : m_nodes.values(group)) {
            int nodeX = cos(angle) * offset + cx;
            int nodeY = sin(angle) * offset + cy;
            nodeAngles[node] = angle;
            m_positions.insert(node, QPoint(nodeX, nodeY));
            m_nodeColors.insert(node, groupColors[group]);

            offset += offsetStep;
        }
        angle += angleStep;
    }

    QPainterPathStroker stroker;
    stroker.setWidth(1.1);

    int lineAlpha = 192;
    if (m_edges.count() > 1000) {
        lineAlpha = 32;
    } else if (m_edges.count() > 100) {
        lineAlpha = 64;
    }

    for (Edge &edge : m_edges) {
        const double nodeX = m_positions[edge.source].x();
        const double nodeY = m_positions[edge.source].y();
        const double otherX = m_positions[edge.target].x();
        const double otherY = m_positions[edge.target].y();

        double magnitude = hypot(nodeX - cx, nodeY - cy);
        double otherMagnitude = hypot(otherX - cx, otherY - cy);
        double averageRadians = atan2(((nodeY - cy) + (otherY - cy))/2, ((nodeX - cx) + (otherX - cx))/2);

        if (nodeAngles[edge.source] == nodeAngles[edge.target]) {
            averageRadians += (magnitude - otherMagnitude) / maxLength;
        }

        double averageMagnitude;
        if (m_scaleEdgeMax) {
            averageMagnitude = qMax(magnitude, otherMagnitude);
        } else {
            averageMagnitude = (magnitude + otherMagnitude) / 2;
        }

        QPointF controlPoint(cos(averageRadians) * averageMagnitude + cx, sin(averageRadians) * averageMagnitude + cy);

        QLinearGradient gradient(m_positions[edge.source], m_positions[edge.target]);
        QColor color = m_nodeColors[edge.source];
        color.setAlpha(lineAlpha);
        gradient.setColorAt(0, color);
        color.setAlpha(lineAlpha / 2);
        gradient.setColorAt(0.5, color);
        color = m_nodeColors[edge.target];
        color.setAlpha(lineAlpha / 3);
        gradient.setColorAt(1, color);
        edge.brush = QBrush(gradient);

        QPainterPath path;
        path.moveTo(m_positions[edge.source]);
        path.quadTo(controlPoint, m_positions[edge.target]);
        edge.path = stroker.createStroke(path);
    }
    qDebug() << "calculating took" << timer.restart() << "ms";
}
