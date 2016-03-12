#ifndef REPLICODEHANDLER_H
#define REPLICODEHANDLER_H

#include <QObject>
#include <QTextDocument>
#include <r_comp/segments.h>
#include <r_comp/decompiler.h>
#include "hivewidget.h"

class ReplicodeHandler : public QObject
{
    Q_OBJECT
public:
    explicit ReplicodeHandler(QObject *parent = 0);
    ~ReplicodeHandler();

    const QMultiMap<QString, QString> &getNodes() { return m_nodes; }
    const QList<Edge> &getEdges() { return m_edges; }
    const QMap<QString, QTextDocument*> getSourceCode() { return m_sourceCode; }

    void loadImage(QString file);

signals:
    void error(QString error);

private:
    r_comp::Image *m_image;
    r_comp::Metadata *m_metadata;
    r_comp::Decompiler *m_decompiler;
    QMultiMap<QString, QString> m_nodes;
    QList<Edge> m_edges;
    QMap<QString, QTextDocument*> m_sourceCode;

};

#endif // REPLICODEHANDLER_H
