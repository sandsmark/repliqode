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
      m_scaleAxis(true),
      m_renderTime(0)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    setMouseTracking(true);
}

HiveWidget::~HiveWidget()
{

}

void HiveWidget::setNodes(const QMap<QString, Node> &nodes)
{
    m_nodes = nodes;
    m_disabledGroups.clear();
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

    QFontMetrics fontMetrics(font());
    QString fpsMessage = QString("%1 ms rendertime").arg(m_renderTime);
    painter.drawText(width() - fontMetrics.horizontalAdvance(fpsMessage) - 10, height() - fontMetrics.height() / 4, fpsMessage);

    QRect groupRect;
    groupRect.moveRight(m_groupsXOffset);
    groupRect.setHeight(fontMetrics.height());
    groupRect.setWidth(width() - m_groupsXOffset);

    for (const QString &groupName : m_groupColors.keys()) {
        if (m_disabledGroups.contains(groupName)) {
            painter.setPen(Qt::gray);
        } else {
            painter.setPen(m_groupColors.value(groupName));
        }
        painter.drawText(groupRect, Qt::AlignVCenter | Qt::AlignLeft, groupName);
        groupRect.moveTop(m_groupYPositions.value(groupName));
    }

    // Draw underlying edges first
    painter.setPen(Qt::NoPen);
    for (const Edge &edge : m_edges) {
        if (m_disabledGroups.contains(m_nodes[edge.source].subgroup) || m_disabledGroups.contains(m_nodes[edge.target].subgroup)) {
            continue;
        }
        if (edge.source == m_closest) {
            continue;
        }
        if (m_closest.isEmpty()) {
            painter.setBrush(edge.highlightBrush);
        } else {
            painter.setBrush(edge.brush);
        }
        painter.drawPath(edge.path);
    }

    // Draw nodes
    QPen nodePen;
    nodePen.setWidth(5);
    if (m_closest.isEmpty()) {
        for (const QString &nodeName : m_nodes.keys()) {
            const Node &node = m_nodes.value(nodeName);
            if (m_disabledGroups.contains(node.subgroup)) {
                continue;
            }

            QColor color(node.color);
            color.setAlpha(128);
            nodePen.setColor(color);
            painter.setPen(nodePen);
            painter.drawPoint(node.x, node.y);
            painter.drawText(node.x, node.y, node.displayName);
        }
    }
    painter.setPen(Qt::NoPen);

    if (m_closest.isEmpty()) {
        m_renderTime = timer.elapsed();
        return;
    }


    // Draw active edges on top
    for (const Edge &edge : m_edges) {
        if (m_disabledGroups.contains(m_nodes[edge.source].subgroup) || m_disabledGroups.contains(m_nodes[edge.target].subgroup)) {
            continue;
        }
        if (edge.source == m_closest) {
            QColor color;
            if (edge.isView) {
                color = QColor(Qt::white);
            } else {
                color = m_nodes.value(edge.source).color;
            }
            color.setAlpha(192);
            painter.setBrush(color);
            painter.drawPath(edge.path);
            painter.drawPolygon(edge.arrowhead);
        }

        // Draw twice, for subtle highlight
        if (edge.target == m_closest) {
            painter.setBrush(edge.highlightBrush);
            painter.drawPath(edge.path);
            painter.drawPolygon(edge.arrowhead);
        }
    }

    QColor penColor(Qt::white);
    const Node &closest = m_nodes.value(m_closest);
    painter.setBrush(closest.color);
    painter.drawEllipse(closest.x - 5, closest.y - 5, 10, 10);
    painter.setPen(penColor);
    painter.drawText(closest.x + 5, closest.y, closest.displayName);

    // Draw text and highlight positions of related edges
    for (const Edge &edge : m_edges) {
        if (m_disabledGroups.contains(m_nodes[edge.source].subgroup) || m_disabledGroups.contains(m_nodes[edge.target].subgroup)) {
            continue;
        }
        if (edge.source == m_closest) {
            const Node &node = m_nodes.value(edge.target);
            penColor.setAlpha(192);
            painter.setPen(penColor);
            painter.drawText(node.x + 10, node.y + 5, node.displayName);
        } else if (edge.target == m_closest) {
            const Node &node = m_nodes.value(edge.source);
            penColor.setAlpha(128);
            painter.setPen(penColor);
            painter.drawText(node.x, node.y, node.displayName);
        }
    }

    // Draw source code of current node
    if (m_nodes.value(m_closest).sourcecode) {
        m_nodes.value(m_closest).sourcecode->drawContents(&painter);
    }

    m_renderTime = timer.elapsed();
}

QString HiveWidget::getClosest(int x, int y)
{
    double minDist = width();
    QString closest;
    for (const QString &nodeName : m_nodes.keys()) {
        if (m_disabledGroups.contains(m_nodes[nodeName].subgroup)) {
            continue;
        }

        int nodeX = m_nodes.value(nodeName).x;
        int nodeY = m_nodes.value(nodeName).y;
        double dist = hypot(x - nodeX, y - nodeY);
        if (dist < 100 && dist < minDist) {
            minDist = dist;
            closest = nodeName;
        }
    }
    return closest;
}

void HiveWidget::mouseMoveEvent(QMouseEvent *event)
{
    QString closest = getClosest(event->x(), event->y());
    if (closest.isEmpty()) {
        closest = m_clicked;
    }
    if (closest != m_closest) {
        m_closest = closest;
        update();
    }
}

void HiveWidget::mousePressEvent(QMouseEvent *event)
{
    // Check if clicked on group
    QFontMetrics fontMetrics(font());
    QRect groupRect;
    groupRect.moveRight(m_groupsXOffset);
    groupRect.setHeight(fontMetrics.height());
    groupRect.setWidth(width() - m_groupsXOffset);
    for (const QString &groupName : m_groupYPositions.keys()) {
        if (groupRect.contains(event->pos())) {
            if (m_disabledGroups.contains(groupName)) {
                m_disabledGroups.removeAll(groupName);
            } else {
                m_disabledGroups.append(groupName);
            }
            calculate();
            update();
            return;
        }
        groupRect.moveTop(m_groupYPositions.value(groupName));
    }


    QString clicked = getClosest(event->x(), event->y());
    if (clicked != m_clicked) {
        m_clicked = clicked;
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

    if (m_nodes.isEmpty() || m_edges.isEmpty()) {
        return;
    }

    // Get all groups and subgroups
    QSet<QString> groupSet;
    QSet<QString> subgroupSet;
    QMap<QString, int> groupNumElements;
    for (const Node &node : m_nodes.values()) {
        groupSet.insert(node.group);
        subgroupSet.insert(node.subgroup);
        groupNumElements[node.group]++;
    }

    QStringList groups = groupSet.values();
    std::sort(groups.begin(), groups.end());
    QStringList subgroups = subgroupSet.values();
    std::sort(subgroups.begin(), subgroups.end());

    QList<int> groupCounts = groupNumElements.values();
    int maxGroupSize = *std::max_element(groupCounts.begin(), groupCounts.end());


    // For calculating group legend positions
    QFontMetrics fontMetrics(font());
    int maxWidth = 0;
    int textY = 20;

    // Automatically generate some colors
    const int hueStep = 359 / subgroups.count();
    int hue = 0;
    for(const QString &subgroup : subgroups) {
        maxWidth = qMax(maxWidth, fontMetrics.horizontalAdvance(subgroup));
        m_groupYPositions.insert(subgroup, textY);
        textY += fontMetrics.height();

        m_groupColors.insert(subgroup, QColor::fromHsv(hue, 128, 255));
        hue += hueStep;
    }
    m_groupsXOffset = width() - maxWidth;

    // Calculate some angles
    const int cx = width() / 2;
    const int cy =  height() / 1.75;
    const double angleStep = (M_PI * 2) / groups.count();
    const double axisLength = height() / 1.75 - 50;
    double angle = M_PI / 6;
    QHash<QString, double> groupAngles;
    QHash<QString, double> axisOffsets;
    for(const QString &group : groups) {
        groupAngles[group] = angle;
        axisOffsets[group] = 50;
        angle += angleStep;
    }

    for (const QString &nodeName : m_nodes.keys()) {
        Node &node = m_nodes[nodeName];
        if (m_disabledGroups.contains(node.subgroup)) {
            continue;
        }

        node.x = cos(groupAngles[node.group]) * axisOffsets[node.group] + cx;
        node.y = sin(groupAngles[node.group]) * axisOffsets[node.group] + cy;
        node.color = m_groupColors.value(node.subgroup);

        double offsetStep;
        if (m_scaleAxis) {
            offsetStep = axisLength / (groupNumElements.value(node.group) + 1);
        } else {
            offsetStep = (axisLength - 100) / maxGroupSize;
        }
        axisOffsets[node.group] += offsetStep;
    }

    QPainterPathStroker stroker;
    stroker.setWidth(1);

    int lineAlpha = 64;

    for (Edge &edge : m_edges) {
        const Node &node = m_nodes.value(edge.source);
        const Node &otherNode = m_nodes.value(edge.target);
        if (m_disabledGroups.contains(node.subgroup) || m_disabledGroups.contains(otherNode.subgroup)) {
            continue;
        }

        const double nodeX = node.x;
        const double nodeY = node.y;
        const double otherX = otherNode.x;
        const double otherY = otherNode.y;

        double magnitude = hypot(nodeX - cx, nodeY - cy);
        double otherMagnitude = hypot(otherX - cx, otherY - cy);
        double averageRadians = atan2(((nodeY - cy) + (otherY - cy))/2, ((nodeX - cx) + (otherX - cx))/2);

        if (groupAngles[edge.source] == groupAngles[edge.target]) {
            averageRadians += (magnitude - otherMagnitude) / axisLength;
        } else if (fmod(groupAngles[edge.source], M_PI) == fmod(groupAngles[edge.target], M_PI)) {
            averageRadians += (magnitude - otherMagnitude) / axisLength;
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
            color.setAlpha(lineAlpha / 2);
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

        const double sourceAngle = atan2(controlPoint.y() - otherY, controlPoint.x() - otherX);
        QPoint endPoint = QPoint(cos(sourceAngle) * 5 + otherX, sin(sourceAngle) * 5 + otherY);
        QPainterPath path;
        path.moveTo(nodeX, nodeY);
        path.quadTo(controlPoint, QPoint(otherX, otherY));
        edge.path = stroker.createStroke(path);

        // Draw an arrowhead
        const double arrowSize = 25.;
        double arrowAngle = sourceAngle - M_PI / 20;
        QPoint arrowHeadLeft = QPoint(otherX + cos(arrowAngle) * arrowSize,
                                      otherY + sin(arrowAngle) * arrowSize);

        arrowAngle = sourceAngle + M_PI / 20;
        QPoint arrowHeadRight = QPoint(otherX + cos(arrowAngle) * arrowSize,
                                       otherY + sin(arrowAngle) * arrowSize);

        edge.arrowhead = QPolygon();
        edge.arrowhead << endPoint << arrowHeadLeft << arrowHeadRight;
    }
    if (timer.elapsed() > 0) {
        qDebug() << "calculating took" << timer.restart() << "ms";
    }
}
