#ifndef HIVEWIDGET_H
#define HIVEWIDGET_H

#include <QOpenGLWidget>
#include <QMultiMap>
#include <QTextDocument>
#include <memory>

struct Node {
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
    QString m_closest;
    QString m_clicked;
    bool m_scaleEdgeMax;
    bool m_scaleAxis;
};

#endif // HIVEWIDGET_H
