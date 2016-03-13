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
                color = m_nodeColors[edge.source];
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
    painter.setBrush(m_nodeColors[m_closest]);
    painter.drawEllipse(m_positions[m_closest].x() - 5, m_positions[m_closest].y() - 5, 10, 10);
    painter.setPen(penColor);
    int posX = m_positions[m_closest].x();
    int posY = m_positions[m_closest].y();
    painter.drawText(posX + 5, posY, m_closest);

    // Draw text and highlight positions of related edges
    for (const Edge &edge : m_edges) {
        if (edge.source == m_closest) {
            penColor.setAlpha(192);
            painter.setPen(penColor);
            QPoint position = m_positions[edge.target];
            position.setX(position.x() + 10);
            position.setY(position.y() + 5);
            painter.drawText(position, edge.target);
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
    stroker.setWidth(1);

    int lineAlpha = 64;

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

        // Create normal background brush
        if (edge.isView) {
            QColor color(Qt::white);
            color.setAlpha(lineAlpha / 3);
            edge.brush = QBrush(color);
        } else {
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
        }

        // Create more prominent highlighting brush
        if (edge.isView) {
            QColor color(Qt::white);
            color.setAlpha(lineAlpha);
            edge.highlightBrush = QBrush(color);
        } else {
            QLinearGradient gradient(m_positions[edge.source], m_positions[edge.target]);
            QColor color = m_nodeColors[edge.source];
            color.setAlpha(lineAlpha);
            gradient.setColorAt(0, color);
            color.setAlpha(lineAlpha);
            gradient.setColorAt(0.8, color);
            color = m_nodeColors[edge.target];
            color.setAlpha(lineAlpha / 1.5);
            gradient.setColorAt(1, color);
            edge.highlightBrush = QBrush(gradient);
        }

        QPainterPath path;
        path.moveTo(m_positions[edge.source]);
        path.quadTo(controlPoint, m_positions[edge.target]);

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
        path.moveTo(m_positions[edge.target]);
        path.lineTo(arrowHeadLeft);
        path.lineTo(arrowHeadRight);
        path.lineTo(m_positions[edge.target]);

        edge.path = path;
    }
    qDebug() << "calculating took" << timer.restart() << "ms";
}
