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
#include <QListWidget>

Window::Window(QWidget *parent) : QWidget(parent),
    m_hivePlot(new HiveWidget(this)),
    m_replicode(new ReplicodeHandler(this)),
    m_loadImageButton(new QPushButton("Load &image...", this)),
    m_loadSourceButton(new QPushButton("&Load source...", this)),
    m_runButton(new QPushButton("&Run", this)),
    m_outputView(new QTextEdit),
    m_groupList(new QListWidget),
    m_debugStream(std::cout),
    m_errorStream(std::cerr)
{
    m_textColor = palette().color(QPalette::Active, QPalette::ButtonText);
    connect(&m_debugStream, &StreamRedirector::stringOutput, this, [=](QString string) {
            m_outputView->setTextColor(m_textColor);
            m_outputView->insertPlainText(string);
            m_outputView->ensureCursorVisible();
        }, Qt::QueuedConnection);
    connect(&m_errorStream, &StreamRedirector::stringOutput, this, [=](QString string) {
            m_outputView->setTextColor(Qt::red);
            m_outputView->insertPlainText(string);
            m_outputView->ensureCursorVisible();
        }, Qt::QueuedConnection);

    m_runButton->setCheckable(true);
    QPushButton *clearButton = new QPushButton("Clear");
    connect(clearButton, &QPushButton::clicked, m_outputView, &QTextEdit::clear);

    connect(m_replicode, &ReplicodeHandler::error, this, &Window::onReplicodeError);
    connect(m_loadImageButton, &QPushButton::clicked, this, &Window::onLoadImage);
    connect(m_loadSourceButton, &QPushButton::clicked, this, &Window::onLoadSource);
    connect(m_runButton, &QPushButton::clicked, this, &Window::onRunClicked);
    connect(m_groupList, &QListWidget::itemChanged, this, &Window::onGroupClicked);

    QHBoxLayout *l = new QHBoxLayout;
    setLayout(l);
    l->addWidget(m_hivePlot, 3);

    QWidget *rightWidget = new QWidget;
    rightWidget->setLayout(new QVBoxLayout);
    m_outputView->setReadOnly(true);

    rightWidget->layout()->addWidget(m_runButton);
    rightWidget->layout()->addWidget(m_groupList);
    rightWidget->layout()->addWidget(m_outputView);
    rightWidget->layout()->addWidget(clearButton);
    rightWidget->layout()->addWidget(m_loadSourceButton);
    rightWidget->layout()->addWidget(m_loadImageButton);
    l->addWidget(rightWidget, 1);

//    QSettings settings;
//    QString lastImageFile = settings.value("lastimage").toString();
//    if (QFile::exists(lastImageFile)) {
//        m_replicode->loadImage(lastImageFile);
//    }

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
    loadNodes();
}

void Window::onLoadSource()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select a source file", "", "*.replicode");
    if (!QFile::exists(filePath)) {
        return;
    }
    m_replicode->loadSource(filePath);
    loadNodes();
}

void Window::onRunClicked(bool checked)
{
    if (checked) {
        qDebug() << "Starting...";
        if (!m_replicode->start()) {
            m_runButton->setChecked(false);
        }
        m_runButton->setText("&Stop");
    } else {
        qDebug() << "Stopping...";
        m_replicode->stop();
        loadNodes();
        m_runButton->setText("&Run");
    }
}

void Window::onReplicodeError(QString error)
{
    QMessageBox::warning(this, "Replicode error", error);
}

void Window::onGroupClicked(QListWidgetItem *item)
{
    if (item->checkState() == Qt::Unchecked && !m_disabledGroups.contains(item->text())) {
        m_disabledGroups.append(item->text());
        m_hivePlot->setDisabledGroups(m_disabledGroups);
    } else if (item->checkState() == Qt::Checked && m_disabledGroups.contains(item->text())) {
        m_disabledGroups.removeAll(item->text());
        m_hivePlot->setDisabledGroups(m_disabledGroups);
    }
}

void Window::loadNodes()
{
    const QMap<QString, Node> nodes = m_replicode->getNodes();
    m_groupList->clear();

    QSet<QString> subgroupSet;
    for (const Node &node : nodes.values()) {
        subgroupSet.insert(node.subgroup);
    }

    QStringList subgroups = subgroupSet.toList();
    qSort(subgroups);
    for (const QString group : subgroups) {
        QListWidgetItem *item = new QListWidgetItem(group, m_groupList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
    }
    m_disabledGroups.clear();

    m_hivePlot->setDisabledGroups(m_disabledGroups);
    m_hivePlot->setNodes(nodes);
    m_hivePlot->setEdges(m_replicode->getEdges());
}
