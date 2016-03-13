#ifndef REPLICODEHIGHLIGHTER_H
#define REPLICODEHIGHLIGHTER_H

#include <QSyntaxHighlighter>

class QTextDocument;

class ReplicodeHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    
    struct Rule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };

public:
    ReplicodeHighlighter(QTextDocument *parent = 0 );
    
protected:
    virtual void highlightBlock(const QString &block );
    
private:
    QVector<Rule> createRules(const QStringList &keywords, const QTextCharFormat &format);

    QVector<Rule> m_rules;
};

#endif//REPLICODEHIGHLIGHTER_H
