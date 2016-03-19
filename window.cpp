#include "window.h"
#include "hivewidget.h"
#include "replicodehandler.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QFile>
#include <QTextEdit>

Window::Window(QWidget *parent) : QWidget(parent),
    m_hivePlot(new HiveWidget(this)),
    m_replicode(new ReplicodeHandler(this)),
    m_button(new QPushButton("Load file...", this)),
    m_outputView(new QTextEdit)
{
    connect(m_replicode, &ReplicodeHandler::error, this, &Window::onReplicodeError);
    connect(m_button, &QPushButton::clicked, this, &Window::buttonClicked);

    QHBoxLayout *l = new QHBoxLayout;
    setLayout(l);
    l->addWidget(m_hivePlot, 3);

    QWidget *rightWidget = new QWidget;
    rightWidget->setLayout(new QVBoxLayout);
    m_outputView->setReadOnly(true);

    rightWidget->layout()->addWidget(m_outputView);
    rightWidget->layout()->addWidget(m_button);
    l->addWidget(rightWidget, 1);

    QSettings settings;
    QString lastImageFile = settings.value("lastimage").toString();
    if (QFile::exists(lastImageFile)) {
        m_replicode->loadImage(lastImageFile);
    }

    m_hivePlot->setNodes(m_replicode->getNodes());
    m_hivePlot->setEdges(m_replicode->getEdges());
    layout()->setContentsMargins(0, 0, 0, 0);
}

void Window::buttonClicked()
{
    QSettings settings;
    QString lastImageFile = settings.value("lastimage").toString();
    QString filePath = QFileDialog::getOpenFileName(this, "Select an image", lastImageFile, "*.image");
    if (!QFile::exists(filePath)) {
        return;
    }

    settings.setValue("lastimage", filePath);

    m_replicode->loadImage(filePath);
    m_hivePlot->setNodes(m_replicode->getNodes());
    m_hivePlot->setEdges(m_replicode->getEdges());
}

void Window::onReplicodeError(QString error)
{
    QMessageBox::warning(this, "Replicode error", error);
}
