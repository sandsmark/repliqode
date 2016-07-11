#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include "streamredirector.h"

class HiveWidget;
class ReplicodeHandler;
class QPushButton;
class QTextEdit;
class QListWidget;
class QListWidgetItem;

class Window : public QWidget
{
    Q_OBJECT
public:
    explicit Window(QWidget *parent = 0);

signals:

private slots:
    void onLoadImage();
    void onLoadSource();
    void onRunClicked(bool checked);
    void onReplicodeError(QString error);
    void onGroupClicked(QListWidgetItem *item);

private:
    void loadNodes();

    HiveWidget *m_hivePlot;
    ReplicodeHandler *m_replicode;
    QPushButton *m_loadImageButton;
    QPushButton *m_loadSourceButton;
    QPushButton *m_runButton;
    QTextEdit *m_outputView;
    QListWidget *m_groupList;
    QStringList m_disabledGroups;
    StreamRedirector m_debugStream;
    StreamRedirector m_errorStream;
    QColor m_textColor;
};

#endif // WINDOW_H
