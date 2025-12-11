#include <chatpanelwidget.h>
#include <leftpanelwidget.h>
#include <mainwindow.h>
#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ---------------- Left ----------------
    auto leftPanel = new LeftPanelWidget(this);
    connect(leftPanel, &LeftPanelWidget::downloadClicked, [/*this*/]() {
        // TODO: open download dialog
        qDebug() << "downloadClicked";
    });
    connect(leftPanel, &LeftPanelWidget::aboutClicked, [/*this*/]() {
        // TODO: show about window
        qDebug() << "aboutClicked";
    });
    connect(leftPanel, &LeftPanelWidget::chatSelected, this, &MainWindow::onChatSelected);
    connect(leftPanel, &LeftPanelWidget::chatRemoved, this, &MainWindow::onChatRemoved);
    mainLayout->addWidget(leftPanel);

    // ---------------- Chat container ----------------
    chatContainer = new QWidget(this);
    mainLayout->addWidget(chatContainer, 1);

    // Create initial chat
    leftPanel->createInitialChat("New LLM chat");
}

MainWindow::~MainWindow()
{
    //
}

void MainWindow::onChatRemoved(QWidget *chatWidget)
{
    // Remove all existing widgets from the container
    if (chatContainer->layout() && !chatContainer->layout()->isEmpty()) {
        QLayoutItem *item;
        while ((item = chatContainer->layout()->takeAt(0)) != nullptr) {
            if (item->widget() == chatWidget) {
                delete item->widget();
                //delete item;
                return;
            }
        }
    }
}

void MainWindow::onChatSelected(QWidget *chatWidget)
{
    // Get the existing layout or create a new one if it doesn't exist
    QVBoxLayout *chatLayout = qobject_cast<QVBoxLayout *>(chatContainer->layout());
    if (!chatLayout) {
        chatLayout = new QVBoxLayout(chatContainer);
        chatLayout->setContentsMargins(0, 0, 0, 0);
    }

    // Remove all existing widgets from the container
    QLayoutItem *item;
    while ((item = chatLayout->takeAt(0)) != nullptr) {
        QWidget *widget = item->widget();
        if (widget && widget != chatWidget) {
            widget->setVisible(false);
            delete item;
        }
    }

    // Add the new chat widget if it's not already in the layout
    if (chatLayout->indexOf(chatWidget) == -1) {
        chatLayout->addWidget(chatWidget);
    }

    // Make sure the selected widget is visible
    chatWidget->setVisible(true);
}

QWidget *MainWindow::getCentralWidget() const
{
    return centralWidget;
}
