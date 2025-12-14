#include <chatlistitemdelegate.h>
#include <chatpanelwidget.h>
#include <leftpanelwidget.h>
#include <mainwindow.h>
#include <QApplication>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStyle>

LeftPanelWidget::LeftPanelWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(150);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(12);

    // ---------------- New Chat ----------------------------------
    newChatButton = new QPushButton(tr("New LLM Chat"), this);
    newChatButton->setMinimumHeight(48);
    connect(newChatButton, &QPushButton::clicked, this, &LeftPanelWidget::onNewChat);
    layout->addWidget(newChatButton);

    // ---------------- Chat List ----------------------------------
    chatModel = new ChatListModel(this);
    connect(chatModel, &ChatListModel::chatWidgetRemoved, this, [this](QWidget *w) { //
        emit chatRemoved(w);
    });
    connect(chatModel, &ChatListModel::chatWidgetAdded, this, [this](QWidget *w) { //
        if (chatModel->rowCount() > 0) {
            // Select the new chat
            QModelIndex newIndex = chatModel->index(chatModel->rowCount() - 1, 0);
            chatList->setCurrentIndex(newIndex);
            // Emit signal to update main window
            emit chatSelected(w);
        }
    });

    chatList = new QListView(this);
    chatList->setSelectionMode(QAbstractItemView::SingleSelection);
    ChatListItemDelegate *delegate = new ChatListItemDelegate(this);
    chatList->setItemDelegate(delegate);
    chatList->setModel(chatModel);

    connect(chatList, &QListView::clicked, this, &LeftPanelWidget::onChatItemClicked);
    connect(delegate, &ChatListItemDelegate::deleteRequested, this, &LeftPanelWidget::onDeleteChatRequested);

    // Enable context menu
    chatList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(chatList, &QWidget::customContextMenuRequested, this, &LeftPanelWidget::onContextMenu);

    layout->addWidget(chatList, 1);

    // ---------------- Trash confirmation -------------------------
    trashConfirmWidget = new QWidget(this);
    trashConfirmWidget->setVisible(false);

    auto trashLayout = new QHBoxLayout(trashConfirmWidget);
    trashLayout->setContentsMargins(0, 0, 0, 0);

    trashYes = new QPushButton(QString::fromUtf8("✓"), trashConfirmWidget);
    trashYes->setObjectName("trashYes");

    trashNo = new QPushButton(QString::fromUtf8("✕"), trashConfirmWidget);
    trashNo->setObjectName("trashNo");

    trashLayout->addWidget(trashYes);
    trashLayout->addWidget(trashNo);

    layout->addWidget(trashConfirmWidget);

    connect(trashYes, &QPushButton::clicked, this, &LeftPanelWidget::onConfirmDelete);
    connect(trashNo, &QPushButton::clicked, this, &LeftPanelWidget::onCancelDelete);

    autoHideTimer = new QTimer(this);
    autoHideTimer->setSingleShot(true);
    autoHideTimer->setInterval(3000);
    connect(autoHideTimer, &QTimer::timeout, this, &LeftPanelWidget::onCancelDelete);

    // ---------------- Updates ------------------------------------
    updatesButton = new QPushButton(tr("Updates"), this);
    updatesButton->setMinimumHeight(48);
    connect(updatesButton, &QPushButton::clicked, this, &LeftPanelWidget::onUpdates);
    layout->addWidget(updatesButton);

    // ---------------- Downloads ----------------------------------
    downloadsButton = new QPushButton(tr("Downloads"), this);
    downloadsButton->setMinimumHeight(48);
    connect(downloadsButton, &QPushButton::clicked, this, &LeftPanelWidget::downloadClicked);
    layout->addWidget(downloadsButton);

    // ---------------- About --------------------------------------
    aboutButton = new QPushButton(tr("About"), this);
    aboutButton->setMinimumHeight(48);
    connect(aboutButton, &QPushButton::clicked, this, &LeftPanelWidget::aboutClicked);
    layout->addWidget(aboutButton);
}

static inline MainWindow *mainWindow()
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

void LeftPanelWidget::createChatWidget(const QString &name)
{
    if (MainWindow *mw = mainWindow()) {
        ChatPanelWidget *newWidget = new ChatPanelWidget( //
            mw->syntaxModel(),
            mw->toolModel(),
            mw->contentWidget());

        // Create initial chat
        chatModel->addChat(name, newWidget);
    }
}

void LeftPanelWidget::onChatItemClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    // Get the chat widget from the model
    ChatListModel::ChatData *chatData = chatModel->getChatData(index.row());
    if (chatData) {
        emit chatSelected(chatData->widget);
    }
}

void LeftPanelWidget::onEditChat()
{
    auto currentIndex = chatList->currentIndex();
    if (!currentIndex.isValid())
        return;

    QString currentName = currentIndex.data(Qt::DisplayRole).toString();

    bool ok = false;
    QString newName = QInputDialog::getText( //
        this,
        tr("Rename chat"),
        tr("Enter name:"),
        QLineEdit::Normal,
        currentName,
        &ok);
    if (ok) {
        chatModel->setData(currentIndex, newName, Qt::EditRole);
    }
}

void LeftPanelWidget::onDeleteChat()
{
    auto currentIndex = chatList->currentIndex();
    if (!currentIndex.isValid())
        return;

    pendingDeleteIndex = currentIndex.row();
    trashConfirmWidget->setVisible(true);
    autoHideTimer->start();
}

void LeftPanelWidget::onDeleteChatRequested(int row)
{
    pendingDeleteIndex = row;
    trashConfirmWidget->setVisible(true);
    autoHideTimer->start();
}

void LeftPanelWidget::onConfirmDelete()
{
    if (pendingDeleteIndex >= 0 && pendingDeleteIndex < chatModel->chatCount()) {
        chatModel->removeChat(pendingDeleteIndex);
    }

    pendingDeleteIndex = -1;
    trashConfirmWidget->setVisible(false);
}

void LeftPanelWidget::onCancelDelete()
{
    pendingDeleteIndex = -1;
    trashConfirmWidget->setVisible(false);
}

void LeftPanelWidget::onUpdates()
{
    // if (!LLM.checkForUpdates()) show error dialog
}

void LeftPanelWidget::onChatNameChanged(const QString & /*newName*/)
{
    // This slot can be used to update the chat name in the model
    // when it's changed from outside
}

void LeftPanelWidget::onNewChat()
{
    QString newName = tr("New LLM chat");
    int count = chatModel->chatCount();
    if (count > 0) {
        newName = QString(tr("New LLM chat %1")).arg(count + 1);
    }
    createChatWidget(newName);
}

void LeftPanelWidget::onContextMenu(const QPoint &point)
{
    // Create context menu
    QMenu contextMenu(this);

    // Add actions to the context menu
    QAction *addAction = contextMenu.addAction(tr("Add Chat"));
    QAction *editAction = contextMenu.addAction(tr("Edit Chat"));
    QAction *deleteAction = contextMenu.addAction(tr("Delete Chat"));

    // Enable/disable actions based on selection
    auto currentIndex = chatList->currentIndex();
    editAction->setEnabled(currentIndex.isValid());
    deleteAction->setEnabled(currentIndex.isValid());

    // Show the context menu
    QAction *selectedAction = contextMenu.exec(chatList->mapToGlobal(point));

    if (selectedAction == addAction) {
        onNewChat();
    } else if (selectedAction == editAction) {
        onEditChat();
    } else if (selectedAction == deleteAction) {
        onDeleteChat();
    }
}

void LeftPanelWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        // Handle delete/backspace key press
        onDeleteChat();
    } else {
        // Call the parent implementation for other keys
        QWidget::keyPressEvent(event);
    }
}
