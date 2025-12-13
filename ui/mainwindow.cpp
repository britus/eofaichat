#include <chatpanelwidget.h>
#include <leftpanelwidget.h>
#include <mainwindow.h>
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
{
    setWindowTitle(qApp->applicationDisplayName());
    setWindowIcon(QIcon(":/Assets/Icons/icon_512x512.png"));
    setWindowFlag(Qt::WindowType::Window, true);

    QDir cfgDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    QString orgName = qApp->organizationName();
    QString appName = qApp->applicationName();
    QString fileName = cfgDir.absoluteFilePath("eofaichat.settings");

    settings = new QSettings(orgName, appName, this);
    settings->setPath(QSettings::NativeFormat, QSettings::UserScope, fileName);

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
    statusbar->showMessage(tr("%1 | %2 | Copyright © 2025 by EoF Software Labs") //
                               .arg(qApp->applicationDisplayName(), qApp->applicationVersion()));
    setStatusBar(statusbar);

    // center window on primary screen
    QScreen *screen = qApp->primaryScreen();
    int width = settings->value("window.width", 1024).toInt();
    int hight = settings->value("window.height", 640).toInt();
    if (width > 0 && width < screen->size().width() && hight > 0 && hight < screen->size().height()) {
        uint centerX = screen->size().width() / 2 - width / 2;
        uint centerY = screen->size().height() / 2 - hight / 2;
        setGeometry(centerX, centerY, width, hight);
    }

    QSplitter *splitter = new QSplitter(Qt::Horizontal, centralWidget);
    centralWidget->layout()->addWidget(splitter);
    splitter->insertWidget(0, leftPanel);
    splitter->insertWidget(1, chatContainer);
    splitter->setHandleWidth(10);

    int spleft = settings->value("splitter.left", 200).toInt();
    int spright = settings->value("splitter.right", width / 2).toInt();
    splitter->setSizes(QList<int>() << spleft << spright);

    connect(splitter, &QSplitter::splitterMoved, this, [this, splitter](int, int) {
        settings->setValue("splitter.left", splitter->sizes().at(0));
        settings->setValue("splitter.right", splitter->sizes().at(1));
    });

    connect(qApp, &QApplication::aboutToQuit, this, [this] {
        settings->setValue("window.width", geometry().width()); //
        settings->setValue("window.height", geometry().height());
        settings->sync();
        if (settings->status() != QSettings::NoError) {
            qCritical("Unable to write settings file.");
        }
    });

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
