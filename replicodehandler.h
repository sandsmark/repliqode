#ifndef REPLICODEHANDLER_H
#define REPLICODEHANDLER_H

#include <QObject>
#include <QTextDocument>
#include <r_exec/mem.h>
#include <r_comp/segments.h>
#include <r_comp/decompiler.h>
#include "hivewidget.h"

class ReplicodeHandler : public QObject
{
    Q_OBJECT
public:
    explicit ReplicodeHandler(QObject *parent = 0);
    ~ReplicodeHandler();

    const QMap<QString, Node> &getNodes() { return m_nodes; }
    const QList<Edge> &getEdges() { return m_edges; }

    void loadImage(QString file);
    void loadSource(QString file);
    void stop();

public slots:
    void start();

signals:
    void error(QString error);

private:
    void decompileImage(r_comp::Image *image);
    bool initialize();

    r_exec::_Mem *m_mem;
    r_comp::Image *m_image;
    r_comp::Metadata *m_metadata;
    QMap<QString, Node> m_nodes;
    QList<Edge> m_edges;
    bool m_initSuccess;
};

#endif // REPLICODEHANDLER_H
