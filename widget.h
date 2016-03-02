#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMultiMap>

struct Edge {
    QString nodeA;
    QString nodeB;
    QString attributes;
    bool operator==(const Edge &other) {
        return (nodeA == other.nodeA && nodeB == other.nodeB && attributes == other.attributes);
    }
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mouseMoveEvent(QMouseEvent *event);

private:
    QPoint getNodePosition(QString node);

    QMap<QString, QList<QString>> m_nodes;
    //QList<Edge> m_edges;
    QMultiMap<QString, QString> m_edges;
    QMap<QString, QColor> m_colors;
    QMap<QString, QPoint> m_positions;
    QString m_closest;
    int m_maxNodes;
};

#endif // WIDGET_H
