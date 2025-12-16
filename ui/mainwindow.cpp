#include <chatpanelwidget.h>
#include <leftpanelwidget.h>
#include <llmconnectionmodel.h>
#include <llmconnectionsdialog.h>
#include <mainwindow.h>
#include <settingsmanager.h>
#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_settingsManager(new SettingsManager(this))
    , m_connectionModel(new LLMConnectionModel(this))
    , m_syntaxModel(new SyntaxColorModel(this))
    , m_toolModel(new ToolModel(this))
{
    setWindowTitle(qApp->applicationDisplayName());
    setWindowIcon(QIcon(":/assets/eofaichat.png"));
    setWindowFlag(Qt::WindowType::Window, true);
    setMinimumSize(QSize(720, 680));

    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

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
    connect(leftPanel, &LeftPanelWidget::chatRemoved, this, &MainWindow::onChatRemoved);
    connect(leftPanel, &LeftPanelWidget::chatSelected, this, &MainWindow::onSwitchChatPanel);
    m_mainLayout->addWidget(leftPanel);

    // ---------------- Chat container ----------------
    m_contentWidget = new QWidget(this);
    m_mainLayout->addWidget(m_contentWidget, 1);

    QStatusBar *statusbar = new QStatusBar(this);
    statusbar->setSizeGripEnabled(true);
    statusbar->showMessage(                                               //
        QStringLiteral("%1 | %2 | Copyright © 2025 by EoF Software Labs") //
            .arg(qApp->applicationDisplayName(), qApp->applicationVersion()));
    setStatusBar(statusbar);

    // Load window size and position
    m_settingsManager->loadWindowSize(this);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, m_centralWidget);
    m_centralWidget->layout()->addWidget(splitter);
    splitter->insertWidget(0, leftPanel);
    splitter->insertWidget(1, m_contentWidget);
    splitter->setHandleWidth(10);

    // Load splitter position
    m_settingsManager->loadSplitterPosition("main", splitter);

    connect(splitter, &QSplitter::splitterMoved, this, [this, splitter](int, int) { //
        m_settingsManager->saveSplitterPosition("main", splitter);
    });
    connect(qApp, &QApplication::aboutToQuit, this, [this] { //
        m_settingsManager->saveWindowSize(this);
        m_connectionModel->saveConnections();
    });

    // Setup menu bar
    setupMenuBar();

    // Create initial chat
    QTimer::singleShot(500, this, [leftPanel]() { //
        leftPanel->onNewChat();
    });
}

MainWindow::~MainWindow()
{
    m_settingsManager->saveWindowSize(this);
}

MainWindow *MainWindow::window()
{
    // Finding the first QMainWindow in QApplication
    const QWidgetList allWidgets = QApplication::topLevelWidgets();
    for (QObject *obj : allWidgets) {
        if (MainWindow *mw = qobject_cast<MainWindow *>(obj)) {
            return mw;
        }
    }
    return nullptr;
}

extern int loadStyleSheet(QApplication *app, const QString &name);

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();

    // Tools menu
    m_toolsMenu = menuBar->addMenu(tr("&Tools"));

    // Manage connections action
    QAction *manageConnectionsAction = m_toolsMenu->addAction(tr("&Manage LLM Connections"));
    manageConnectionsAction->setShortcut(QKeySequence::Preferences);
    connect(manageConnectionsAction, &QAction::triggered, this, &MainWindow::onManageConnections);

    m_toolsMenu->addSeparator();

    // Theme selector
    QAction *action;
    action = m_toolsMenu->addAction(tr("&Default theme"));
    connect(action, &QAction::triggered, this, []() { //
        loadStyleSheet(qApp, "eofaichat");
    });
    action = m_toolsMenu->addAction(tr("&Purple theme"));
    connect(action, &QAction::triggered, this, []() { //
        loadStyleSheet(qApp, "eofaichat_purple");
    });
}

void MainWindow::onManageConnections()
{
    LLMConnectionsDialog dialog(m_connectionModel, this);
    dialog.exec();
}

void MainWindow::onChatRemoved(QWidget *chatWidget)
{
    // Remove all existing widgets from the container
    if (m_contentWidget->layout() && !m_contentWidget->layout()->isEmpty()) {
        QLayoutItem *item;
        while ((item = m_contentWidget->layout()->takeAt(0)) != nullptr) {
            if (item->widget() == chatWidget) {
                delete item->widget();
                //delete item;
                return;
            }
        }
    }
}

void MainWindow::onSwitchChatPanel(QWidget *chatWidget)
{
    // Get the existing layout or create a new one if it doesn't exist
    QVBoxLayout *chatLayout = qobject_cast<QVBoxLayout *>(m_contentWidget->layout());
    if (!chatLayout) {
        chatLayout = new QVBoxLayout(m_contentWidget);
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

    chatWidget->setVisible(true);
}
