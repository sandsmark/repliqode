#include "hivewidget.h"
#include <QDebug>
#include <qmath.h>
#include <QPainter>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <algorithm>

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

void HiveWidget::setNodes(const QMap<QString, Node> &nodes)
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
        int textWidth = fontMetrics.width(group);
        int xOffset = 0;
        int yOffset = 0;
        int degrees = 180 * angle / M_PI;
        if (degrees > 135 && degrees < 225) {
            xOffset = -textWidth - 10;
            yOffset = fontMetrics.height() / 4;
        } else if (degrees > 135 && degrees < 270) {
            xOffset = -textWidth / 2;
            yOffset = -fontMetrics.height() / 4;
        } else if (degrees > 45 && degrees < 135) {
            xOffset = -textWidth / 2;
            yOffset = fontMetrics.height();
        } else {
            xOffset = 10;
            yOffset = fontMetrics.height() / 4;
        }

        color.setAlpha(200);
        painter.setPen(color);
        painter.drawText(cos(angle) * (m_axisLength) + cx + xOffset, sin(angle) * (m_axisLength) + cy + yOffset, group);

        angle += angleStep;
    }

    // Draw nodes
    QPen nodePen;
    nodePen.setWidth(5);
    for (const Node &node : m_nodes.values()) {
        QColor color(node.color);
        color.setAlpha(128);
        nodePen.setColor(color);
        painter.setPen(nodePen);
        painter.drawPoint(node.x, node.y);
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

    if (m_closest.isEmpty()) {
        return;
    }

    // Draw active edges on top
    for (const Edge &edge : m_edges) {
        if (edge.source == m_closest) {
            QColor color;
            if (edge.isView) {
                color = QColor(Qt::white);
            } else {
                color = m_nodes.value(edge.source).color;//m_nodeColors[edge.source];
            }
            color.setAlpha(192);
            painter.setBrush(color);
            painter.drawPath(edge.path);
        }

        // Draw twice, for subtle highlight
        if (edge.target == m_closest) {
            painter.setBrush(edge.highlightBrush);
            painter.drawPath(edge.path);
        }
    }

    QColor penColor(Qt::white);
    const Node &closest = m_nodes.value(m_closest);
    painter.setBrush(closest.color);
    painter.drawEllipse(closest.x - 5, closest.y - 5, 10, 10);
    painter.setPen(penColor);
    painter.drawText(closest.x + 5, closest.y, m_closest);

    // Draw text and highlight positions of related edges
    for (const Edge &edge : m_edges) {
        if (edge.source == m_closest) {
            const Node &node = m_nodes.value(edge.target);
            penColor.setAlpha(192);
            painter.setPen(penColor);
            painter.drawText(node.x + 10, node.y + 5, edge.target);
        } else if (edge.target == m_closest) {
            const Node &node = m_nodes.value(edge.source);
            penColor.setAlpha(128);
            painter.setPen(penColor);
            painter.drawText(node.x, node.y, edge.source);
        }
    }

    // Draw source code of current node
    penColor.setAlpha(128);
    painter.setPen(penColor);
    if (m_nodes.value(m_closest).sourcecode) {
        m_nodes.value(m_closest).sourcecode->drawContents(&painter);
    }

    // Theoretical but whatever
    if (timer.elapsed() > 0) {
        QString fpsMessage = QString("%1 fps").arg(int(1000 / timer.elapsed()));
        painter.drawText(width() - fontMetrics.width(fpsMessage), height() - fontMetrics.height() / 4, fpsMessage);
    }
}

void HiveWidget::mouseMoveEvent(QMouseEvent *event)
{
    double minDist = width();
    QString closest = m_clicked;
    for (const QString &nodeName : m_nodes.keys()) {
        int nodeX = m_nodes.value(nodeName).x;
        int nodeY = m_nodes.value(nodeName).y;
        double dist = hypot(event->x() - nodeX, event->y() - nodeY);
        if (dist < 100 && dist < minDist) {
            minDist = dist;
            closest = nodeName;
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

    if (m_nodes.isEmpty() || m_edges.isEmpty()) {
        return;
    }

    QSet<QString> groups;
    QSet<QString> subgroups;
    QMap<QString, int> groupNumElements;
    for (const Node &node : m_nodes.values()) {
        groups.insert(node.group);
        subgroups.insert(node.subgroup);
        groupNumElements[node.group]++;
    }

    QList<int> groupCounts = groupNumElements.values();
    int maxGroupSize = *std::max_element(groupCounts.begin(), groupCounts.end());

    // Automatically generate some colors
    const int hueStep = 359 / subgroups.count();
    int hue = 0;
    for(const QString &subgroup : subgroups) {
        m_groupColors.insert(subgroup, QColor::fromHsv(hue, 128, 255));
        hue += hueStep;
    }

    // Find the optimal scale/axis length

    const int cx = width() / 2;
    const int cy = height() / 2;
    const double angleStep = (M_PI * 2) / groups.count();
    double angle = M_PI/4;
    m_axisLength = (qMin(width(), height())) / 2;
    for (int i=0; i<groups.count(); i++) {
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

    // Calculate the angles between the axes, initialize the offsets of the nodes
    angle = M_PI/4;
    QHash<QString, double> groupAngles;
    QMap<QString, double> axisOffsets;
    for(const QString &group : groups) {
        groupAngles[group] = angle;
        axisOffsets[group] = 50;
        angle += angleStep;
    }

    for (const QString &nodeName : m_nodes.keys()) {
        Node &node = m_nodes[nodeName];
        double offsetStep;
        if (m_scaleAxis) {
            offsetStep = m_axisLength / (groupNumElements.value(node.group) + 1);
        } else {
            offsetStep = (m_axisLength - 100) / maxGroupSize;
        }
        axisOffsets[node.group] += offsetStep;

        node.x = cos(groupAngles[node.group]) * axisOffsets[node.group] + cx;
        node.y = sin(groupAngles[node.group]) * axisOffsets[node.group] + cy;
        node.color = m_groupColors.value(node.subgroup);
    }

    QPainterPathStroker stroker;
    stroker.setWidth(1);

    int lineAlpha = 64;

    for (Edge &edge : m_edges) {
        const Node &node = m_nodes.value(edge.source);
        const Node &otherNode = m_nodes.value(edge.target);

        const double nodeX = node.x;
        const double nodeY = node.y;
        const double otherX = otherNode.x;
        const double otherY = otherNode.y;

        double magnitude = hypot(nodeX - cx, nodeY - cy);
        double otherMagnitude = hypot(otherX - cx, otherY - cy);
        double averageRadians = atan2(((nodeY - cy) + (otherY - cy))/2, ((nodeX - cx) + (otherX - cx))/2);

        if (groupAngles[edge.source] == groupAngles[edge.target]) {
            averageRadians += (magnitude - otherMagnitude) / m_axisLength;
        } else if (fmod(groupAngles[edge.source], M_PI) == fmod(groupAngles[edge.target], M_PI)) {
            averageRadians += (magnitude - otherMagnitude) / m_axisLength;
        }

        double averageMagnitude;
        if (m_scaleEdgeMax) {
            averageMagnitude = qMax(magnitude, otherMagnitude);
        } else {
            averageMagnitude = (magnitude + otherMagnitude) / 2;
        }

        QPointF controlPoint(cos(averageRadians) * averageMagnitude + cx, sin(averageRadians) * averageMagnitude + cy);

        // Create normal background brush
        if (edge.isView) {
            QColor color(Qt::white);
            color.setAlpha(lineAlpha / 3);
            edge.brush = QBrush(color);
        } else {
            QLinearGradient gradient(nodeX, nodeY, otherX, otherY);
            QColor color = node.color;
            color.setAlpha(lineAlpha);
            gradient.setColorAt(0, color);
            color.setAlpha(lineAlpha / 2);
            gradient.setColorAt(0.8, color);
            color = otherNode.color;
            color.setAlpha(lineAlpha / 3);
            gradient.setColorAt(1, color);
            edge.brush = QBrush(gradient);
        }

        // Create more prominent highlighting brush
        if (edge.isView) {
            QColor color(Qt::white);
            color.setAlpha(lineAlpha);
            edge.highlightBrush = QBrush(color);
        } else {
            QLinearGradient gradient(nodeX, nodeY, otherX, otherY);
            QColor color = node.color;
            color.setAlpha(lineAlpha);
            gradient.setColorAt(0, color);
            color.setAlpha(lineAlpha);
            gradient.setColorAt(0.8, color);
            color = otherNode.color;
            color.setAlpha(lineAlpha / 1.5);
            gradient.setColorAt(1, color);
            edge.highlightBrush = QBrush(gradient);
        }

        QPainterPath path;
        path.moveTo(nodeX, nodeY);
        path.quadTo(controlPoint, QPoint(otherX, otherY));

        // Draw an arrowhead
        const double arrowSize = 15.;
        const double sourceAngle = atan2(controlPoint.y() - otherY, controlPoint.x() - otherX);
        double arrowAngle = sourceAngle - M_PI / 10;
        QPoint arrowHeadLeft = QPoint(otherX + cos(arrowAngle) * arrowSize,
                                      otherY + sin(arrowAngle) * arrowSize);

        arrowAngle = sourceAngle + M_PI / 10;
        QPoint arrowHeadRight = QPoint(otherX + cos(arrowAngle) * arrowSize,
                                       otherY + sin(arrowAngle) * arrowSize);

        path = stroker.createStroke(path);
        path.moveTo(otherX, otherY);
        path.lineTo(arrowHeadLeft);
        path.lineTo(arrowHeadRight);
        path.lineTo(otherX, otherY);

        edge.path = path;
    }
    qDebug() << "calculating took" << timer.restart() << "ms";
}
