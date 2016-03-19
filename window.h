#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

class HiveWidget;
class ReplicodeHandler;
class QPushButton;
class QTextEdit;

class Window : public QWidget
{
    Q_OBJECT
public:
    explicit Window(QWidget *parent = 0);

signals:

public slots:
private slots:
    void onLoadImage();
    void onLoadSource();
    void onStop();
    void onReplicodeError(QString error);

private:
    HiveWidget *m_hivePlot;
    ReplicodeHandler *m_replicode;
    QPushButton *m_loadImageButton;
    QPushButton *m_loadSourceButton;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
    QTextEdit *m_outputView;
};

#endif // WINDOW_H
