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
    m_loadImageButton(new QPushButton("Load image...", this)),
    m_loadSourceButton(new QPushButton("Load source...", this)),
    m_startButton(new QPushButton("Start", this)),
    m_stopButton(new QPushButton("Stop", this)),
    m_outputView(new QTextEdit)
{
    connect(m_replicode, &ReplicodeHandler::error, this, &Window::onReplicodeError);
    connect(m_loadImageButton, &QPushButton::clicked, this, &Window::onLoadImage);
    connect(m_loadSourceButton, &QPushButton::clicked, this, &Window::onLoadSource);
    connect(m_startButton, &QPushButton::clicked, m_replicode, &ReplicodeHandler::start);
    connect(m_stopButton, &QPushButton::clicked, this, &Window::onStop);

    QHBoxLayout *l = new QHBoxLayout;
    setLayout(l);
    l->addWidget(m_hivePlot, 3);

    QWidget *rightWidget = new QWidget;
    rightWidget->setLayout(new QVBoxLayout);
    m_outputView->setReadOnly(true);

    rightWidget->layout()->addWidget(m_startButton);
    rightWidget->layout()->addWidget(m_stopButton);
    rightWidget->layout()->addWidget(m_outputView);
    rightWidget->layout()->addWidget(m_loadSourceButton);
    rightWidget->layout()->addWidget(m_loadImageButton);
    l->addWidget(rightWidget, 1);

//    QSettings settings;
//    QString lastImageFile = settings.value("lastimage").toString();
//    if (QFile::exists(lastImageFile)) {
//        m_replicode->loadImage(lastImageFile);
//    }

    m_hivePlot->setNodes(m_replicode->getNodes());
    m_hivePlot->setEdges(m_replicode->getEdges());
    layout()->setContentsMargins(0, 0, 0, 0);
}

void Window::onLoadImage()
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

void Window::onLoadSource()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select a source file", "", "*.replicode");
    if (!QFile::exists(filePath)) {
        return;
    }
    m_replicode->loadSource(filePath);
    m_hivePlot->setNodes(m_replicode->getNodes());
    m_hivePlot->setEdges(m_replicode->getEdges());
}

void Window::onStop()
{
    m_replicode->stop();

    m_hivePlot->setNodes(m_replicode->getNodes());
    m_hivePlot->setEdges(m_replicode->getEdges());
}

void Window::onReplicodeError(QString error)
{
    QMessageBox::warning(this, "Replicode error", error);
}
