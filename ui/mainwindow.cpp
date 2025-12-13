#include <chatpanelwidget.h>
#include <leftpanelwidget.h>
#include <mainwindow.h>
#include <settingsmanager.h>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , settingsManager(new SettingsManager(this))
{
    setWindowTitle(qApp->applicationDisplayName());
    setWindowIcon(QIcon(":/assets/eofaichat.png"));
    setWindowFlag(Qt::WindowType::Window, true);

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

    QStatusBar *statusbar = new QStatusBar(this);
    statusbar->setSizeGripEnabled(true);
    statusbar->showMessage(QStringLiteral("%1 | %2 | Copyright © 2025 by EoF Software Labs") //
                               .arg(qApp->applicationDisplayName(), qApp->applicationVersion()));
    setStatusBar(statusbar);

    // Load window size and position
    settingsManager->loadWindowSize(this);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, centralWidget);
    centralWidget->layout()->addWidget(splitter);
    splitter->insertWidget(0, leftPanel);
    splitter->insertWidget(1, chatContainer);
    splitter->setHandleWidth(10);

    // Load splitter position
    settingsManager->loadSplitterPosition(splitter);

    connect(splitter, &QSplitter::splitterMoved, this, [this, splitter](int, int) { //
        settingsManager->saveSplitterPosition(splitter);
    });
    connect(qApp, &QApplication::aboutToQuit, this, [this] { //
        settingsManager->saveWindowSize(this);
    });

    // Create initial chat
    leftPanel->createInitialChat(tr("New LLM chat"));
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
