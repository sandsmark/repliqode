#ifndef HIVEWIDGET_H
#define HIVEWIDGET_H

#include <QOpenGLWidget>
#include <QMultiMap>

struct Edge {
    QString source;
    QString target;
    QPainterPath path;
    QBrush brush;

    bool operator==(const Edge &other) {
        return (source == other.source && target == other.target);
    }
};

class HiveWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    HiveWidget(QWidget *parent = 0);
    ~HiveWidget();

    void setNodes(const QMultiMap<QString, QString> &nodes);
    void setEdges(const QList<Edge> &edges);

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void resizeEvent(QResizeEvent*);

private:
    void calculate();

    QMultiMap<QString, QString> m_nodes;
    QList<Edge> m_edges;

    QMap<QString, QColor> m_nodeColors;
    QMap<QString, QColor> m_colors;
    QMap<QString, QPoint> m_positions;
    QString m_closest;
    bool m_scaleEdgeMax;
};

#endif // HIVEWIDGET_H
