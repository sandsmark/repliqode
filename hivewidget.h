#ifndef HIVEWIDGET_H
#define HIVEWIDGET_H

#include <QOpenGLWidget>
#include <QMultiMap>
#include <QTextDocument>
#include <QPainterPath>
#include <memory>

struct Node {
    QString displayName;
    QString group;
    QString subgroup;
    std::shared_ptr<QTextDocument> sourcecode;

    QColor color;
    int x, y;
};

struct Edge {
    bool isView = false;
    QString source;
    QString target;

    QPainterPath path;
    QPolygon arrowhead;
    QBrush brush;
    QBrush highlightBrush;

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

    void setNodes(const QMap<QString, Node> &nodes);
    void setEdges(const QList<Edge> &edges);

protected:
    virtual void paintEvent(QPaintEvent *) override;
    virtual void mouseMoveEvent(QMouseEvent *) override;
    virtual void mousePressEvent(QMouseEvent*) override;
    virtual void resizeEvent(QResizeEvent*) override;

private:
    void calculate();
    QString getClosest(int x, int y);

    QMap<QString, Node> m_nodes;
    QList<Edge> m_edges;

    QMap<QString, QColor> m_groupColors;
    QMap<QString, int> m_groupYPositions;
    int m_groupsXOffset;
    QString m_closest;
    QString m_clicked;
    bool m_scaleEdgeMax;
    bool m_scaleAxis;
    int m_renderTime;
    QStringList m_disabledGroups;
};

#endif // HIVEWIDGET_H
