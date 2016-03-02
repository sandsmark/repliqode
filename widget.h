#ifndef WIDGET_H
#define WIDGET_H

#include <QOpenGLWidget>
#include <QMultiMap>

struct Edge {
    QString first;
    QString second;
    QPainterPath path;
    QBrush brush;
};

class Widget : public QOpenGLWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void resizeEvent(QResizeEvent*);

private:
    void calculate();

    QList<Edge> m_edgePaths;
    QMap<QString, QColor> m_nodeColors;
    QMap<QString, QList<QString>> m_nodes;
    QMultiMap<QString, QString> m_edges;
    QMap<QString, QColor> m_colors;
    QMap<QString, QPoint> m_positions;
    QString m_closest;
};

#endif // WIDGET_H
