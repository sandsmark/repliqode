#include "replicodehighlighter.h"

#include <QFont>
#include <QStringList>

ReplicodeHighlighter::ReplicodeHighlighter(QTextDocument *parent) :
    QSyntaxHighlighter(parent)
{
    /*******************************
     Rules for keywords and classes
    ********************************/
    QTextCharFormat format;
    QStringList operators;
    operators
            << "_now"
            << "equ"
            << "neq"
            << "gtr"
            << "lsr"
            << "gte"
            << "lse"
            << "add"
            << "sub"
            << "mul"
            << "div"
            << "dis"
            << "ln"
            << "exp"
            << "log"
            << "e10"
            << "syn"
            << "red"
            << "rnd"
            << "fvw";
    format.setForeground(Qt::cyan);
    m_rules.append(createRules(operators, format));

    QStringList builtinClasses;
    builtinClasses
            << "view"
            << "grp_view"
            << "pgm_view"
            << "_obj"
            << "ptn"
            << "\\|ptn"
            << "pgm\\d*"
            << "\\|pgm"
            << "_grp"
            << "grp"
            << "_fact"
            << "fact"
            << "\\|fact"
            << "pred"
            << "goal"
            << "cst"
            << "mdl"
            << "imdl"
            << "icst"
            << "icmd"
            << "cmd"
            << "ent"
            << "ont"
            << "dev"
            << "nod"
            << "ipgm"
            << "icpp_pgm"
            << "perf";
    format.setForeground(Qt::green);
    m_rules.append(createRules(builtinClasses, format));

    QStringList markerClasses;
    markerClasses
            << "mk.rdx\\d*"
            << "mk.val\\d*"
            << "mk.grp_pair"
            << "mk.low_sln"
            << "mk.high_sln"
            << "mk.low_act"
            << "mk.high_act"
            << "mk.low_res"
            << "mk.sln_chg"
            << "mk.act_chg"
            << "mk.new";
    format.setForeground(Qt::darkGreen);
    format.setFontWeight(QFont::Bold);
    m_rules.append(createRules(markerClasses, format));

    QStringList entities;
    entities
            << "self";
    format.setForeground(Qt::green);
    format.setFontWeight(QFont::Bold);
    m_rules.append(createRules(entities, format));

    QStringList groups;
    groups
        << "stdin"
        << "stdout";
    format.setForeground(Qt::darkGreen);
    format.setFontWeight(QFont::Normal);
    m_rules.append(createRules(groups, format));

    QStringList functions;
    functions
            << "_inj"
            << "_eje"
            << "_mod"
            << "_set"
            << "_new_class"
            << "_del_class"
            << "_ldc"
            << "_swp"
            << "_stop";
    format.setForeground(Qt::cyan);
    format.setFontWeight(QFont::Bold);
    m_rules.append(createRules(functions, format));

    QStringList constants;
    constants
            << "\\|nb"
            << "\\|bl"
            << "true"
            << "false"
            << "\\|\\[\\]"
            << "\\|nid"
            << "\\|did"
            << "\\|fid"
            << "\\|st"
            << "\\|us"
            << "forever";
    format.setForeground(Qt::lightGray);
    format.setFontWeight(QFont::Bold);
    m_rules.append(createRules(constants, format));

    /*****************
     Rules for syntax
    ******************/
    Rule rule;
    // Timestamps
    rule.pattern = QRegExp("\\d+s:\\d+ms:\\d+us");
    rule.format.setForeground(Qt::lightGray);
    m_rules.append(rule);

    // Lists
    rule.pattern = QRegExp("\\|?\\[|\\]");
    rule.format.setForeground(Qt::white);
    rule.format.setFontWeight(QFont::Bold);
    m_rules.append(rule);

    // Wildcards
    rule.pattern = QRegExp(" :");
    rule.format.setFontWeight(QFont::Normal);
    rule.format.setForeground(Qt::darkGray);
    m_rules.append(rule);
    rule.pattern = QRegExp(": ");
    m_rules.append(rule);

    // Names
    rule.pattern = QRegExp("[a-zA-Z0-9_\\.]+:[^\\d^ ]");
    rule.format.setForeground(Qt::darkYellow);
    rule.format.setFontWeight(QFont::Normal);
    m_rules.append(rule);
    rule.format.setForeground(Qt::yellow);
    rule.format.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp("\\(|\\)");
    m_rules.append(rule);

    // Comments
    rule.pattern = QRegExp( ";[^\n]*" );
    rule.format.setForeground(Qt::gray);
    rule.format.setFontWeight(QFont::Normal);
    m_rules.append(rule);
}

void ReplicodeHighlighter::highlightBlock(const QString &block)
{
    QTextCharFormat defaultFormat;
    defaultFormat.setForeground(Qt::white);
    setFormat(0, block.length(), defaultFormat);

    for (const Rule &rule : m_rules) {
        QRegExp pattern(rule.pattern);
        int index = pattern.indexIn(block);
        while (index >= 0) {
            int length = pattern.matchedLength();
            setFormat(index, length, rule.format);
            index = pattern.indexIn(block, index + length);
        }
    }
}

QVector<ReplicodeHighlighter::Rule> ReplicodeHighlighter::createRules(const QStringList &keywords, const QTextCharFormat &format)
{
    QVector<Rule> rules;
    Rule rule;
    for (const QString &keyword : keywords) {
        rule.format = format;
        rule.pattern = QRegExp("\\b" + keyword + "\\b");
        rules.append(rule);
    }
    return rules;
}
