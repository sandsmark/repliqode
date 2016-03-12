#include "hivewidget.h"
#include <QDebug>
#include <qmath.h>
#include <QPainter>
#include <QElapsedTimer>
#include <QMouseEvent>

HiveWidget::HiveWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_scaleEdgeMax(false),
      m_scaleAxis(true)
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

void HiveWidget::setSource(const QMap<QString, QTextDocument *> sourcecode)
{
    m_sourcecode = sourcecode;
}

void HiveWidget::paintEvent(QPaintEvent *)
{
    QElapsedTimer timer;
    timer.start();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::black);

    if (m_nodes.isEmpty() || m_edges.isEmpty()) {
        return;
    }

    // Draw axis
    const int cx = width() / 2;
    const int cy = height() / 2;
    const double angleStep = (M_PI * 2) / m_nodes.uniqueKeys().count();
    double angle = M_PI/4;
    QFontMetrics fontMetrics(font());
    for(const QString &group : m_nodes.uniqueKeys()) {
        QColor color = m_groupColors[group];
        color.setAlpha(64);
        painter.setPen(QPen(color, 1));
        painter.drawLine(cx, cy, cos(angle) * m_axisLength + cx, sin(angle) * m_axisLength + cy);

        // Draw name of axis, appropriately positioned
        painter.save();
        int textWidth = fontMetrics.width(group);
        int degrees = 180 * angle / M_PI;
        if (degrees > 90 && degrees < 270) {
            degrees -= 180;
        } else {
            textWidth = 0;
        }

        color.setAlpha(200);
        painter.setPen(color);
        painter.translate(cos(angle) * (m_axisLength + textWidth + 10) + cx, sin(angle) * (m_axisLength + textWidth + 10) + cy);
        painter.rotate(degrees);
        painter.drawText(0, fontMetrics.height() / 4, group);
        painter.restore();

        angle += angleStep;
    }

    // Draw nodes
    QPen nodePen;
    nodePen.setWidth(5);
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
        if (edge.source == m_closest || edge.target == m_closest) {
            continue;
        }
        painter.setBrush(edge.brush);
        painter.drawPath(edge.path);
    }

    if (m_closest.isEmpty()) {
        return;
    }

    // Draw active edges on top
    for (const Edge &edge : m_edges) {
        if (edge.source == m_closest) {
            painter.setBrush(m_nodeColors[m_closest]);
            painter.drawPath(edge.path);
        }
        if (edge.target == m_closest) {
            QColor color = m_nodeColors[edge.source];
            color.setAlpha(128);
            painter.setBrush(color);
            painter.drawPath(edge.path);
        }
    }


    QColor penColor(Qt::white);
    painter.setBrush(m_nodeColors[m_closest]);
    painter.drawEllipse(m_positions[m_closest].x() - 10, m_positions[m_closest].y() - 10, 20, 20);
    painter.setPen(penColor);
    painter.drawText(m_positions[m_closest], m_closest);

    // Draw text and highlight positions of related edges
    for (const Edge &edge : m_edges) {
        if (edge.source == m_closest) {
            QColor ellipseColor = m_nodeColors[m_closest];
            ellipseColor.setAlpha(128);
            painter.setPen(Qt::NoPen);
            painter.setBrush(ellipseColor);
            painter.drawEllipse(m_positions[edge.target].x() - 5, m_positions[edge.target].y() - 5, 10, 10);

            penColor.setAlpha(255);
            painter.setPen(penColor);
            painter.drawText(m_positions[edge.target], edge.target);
        } else if (edge.target == m_closest) {
            penColor.setAlpha(128);
            painter.setPen(penColor);
            painter.drawText(m_positions[edge.source], edge.source);
        }
    }

    // Draw source code of current node
    penColor.setAlpha(128);
    painter.setPen(penColor);
    if (m_sourcecode.contains(m_closest)) {
        m_sourcecode.value(m_closest)->drawContents(&painter);
    }
}

void HiveWidget::mouseMoveEvent(QMouseEvent *event)
{
    double minDist = width();
    QString closest = m_clicked;
    for (const QString node : m_positions.keys()) {
        double dist = hypot(event->x() - m_positions[node].x(), event->y() - m_positions[node].y());
        if (dist < 100 && dist < minDist) {
            minDist = dist;
            closest = node;
        }
    }
    if (closest != m_closest) {
        m_closest = closest;
        update();
    }
}

void HiveWidget::mousePressEvent(QMouseEvent*)
{
    m_clicked = m_closest;
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
    foreach(const QString group, m_nodes.uniqueKeys()) {
        m_groupColors.insert(group, QColor::fromHsv(hue, 128, 255));
        hue += hueStep;

        maxGroupSize = qMax(maxGroupSize, m_nodes.count(group));
    }

    QHash<QString, double> nodeAngles;
    const int cx = width() / 2;
    const int cy = height() / 2;
    const double angleStep = (M_PI * 2) / numGroups;
    double angle = M_PI/4;
    m_axisLength = (qMin(width(), height())) / 2;

    // Find the optimal scale
    for (int i=0; i<numGroups; i++) {
        if (angle > M_PI) {
            angle -= M_PI;
        }
        if (angle < M_PI / 2) {
            m_axisLength = qMin(m_axisLength, cx / cos(angle));
        } else {
            m_axisLength = qMin(m_axisLength, cy / cos(angle - M_PI));
        }
        angle += angleStep;
    }
    m_axisLength -= 50;
    angle = M_PI/4;
    for(const QString &group : m_nodes.uniqueKeys()) {
        double offsetStep;
        if (m_scaleAxis) {
            offsetStep = m_axisLength / (m_nodes.count(group) + 1);
        } else {
            offsetStep = (m_axisLength - 100) / maxGroupSize;
        }

        double offset = offsetStep * 2;
        QStringList nodes = m_nodes.values(group);
        qSort(nodes);
        for (const QString node : nodes) {
            int nodeX = cos(angle) * offset + cx;
            int nodeY = sin(angle) * offset + cy;
            nodeAngles[node] = angle;
            m_positions.insert(node, QPoint(nodeX, nodeY));
            m_nodeColors.insert(node, m_groupColors[group]);

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
        lineAlpha = 192;
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
            averageRadians += (magnitude - otherMagnitude) / m_axisLength;
        } else if (fmod(nodeAngles[edge.source], M_PI) == fmod(nodeAngles[edge.target], M_PI)) {
            averageRadians += (magnitude - otherMagnitude) / m_axisLength;
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
        gradient.setColorAt(0.8, color);
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
