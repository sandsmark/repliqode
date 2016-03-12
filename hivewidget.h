#ifndef HIVEWIDGET_H
#define HIVEWIDGET_H

#include <QOpenGLWidget>
#include <QMultiMap>
#include <QTextDocument>

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

    void setSource(const QMap<QString, QTextDocument*> sourcecode);

protected:
    virtual void paintEvent(QPaintEvent *) override;
    virtual void mouseMoveEvent(QMouseEvent *) override;
    virtual void mousePressEvent(QMouseEvent*) override;
    virtual void resizeEvent(QResizeEvent*) override;

private:
    void calculate();

    QMultiMap<QString, QString> m_nodes;
    QList<Edge> m_edges;

    QMap<QString, QColor> m_nodeColors;
    QMap<QString, QColor> m_groupColors;
    QMap<QString, QPoint> m_positions;
    QString m_closest;
    QString m_clicked;
    double m_axisLength;
    bool m_scaleEdgeMax;
    bool m_scaleAxis;
    QMap<QString, QTextDocument*> m_sourcecode;
};

#endif // HIVEWIDGET_H
