#include <chatlistitemdelegate.h>
#include <chatpanelwidget.h>
#include <leftpanelwidget.h>
#include <llmconnectionselection.h>
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
    , m_chatModel(new ChatListModel(this))
    , m_llmModel(new LLMConnectionModel(this))
{
    setMinimumWidth(150);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(12);

    // ---------------- New Chat ----------------------------------
    m_newChatButton = new QPushButton(tr("New LLM Chat"), this);
    m_newChatButton->setMinimumHeight(48);
    connect(m_newChatButton, &QPushButton::clicked, this, &LeftPanelWidget::onNewChat);
    layout->addWidget(m_newChatButton);

    // ---------------- Chat List ----------------------------------

    connect(m_chatModel, &ChatListModel::chatWidgetRemoved, this, [this](QWidget *w) { //
        emit chatRemoved(w);
    });
    connect(m_chatModel, &ChatListModel::chatWidgetAdded, this, [this](QWidget *w) { //
        if (m_chatModel->rowCount() > 0) {
            // Select the new chat
            QModelIndex newIndex = m_chatModel->index(m_chatModel->rowCount() - 1, 0);
            m_chatList->setCurrentIndex(newIndex);
            // Emit signal to update main window
            emit chatSelected(w);
        }
    });

    m_chatList = new QListView(this);
    m_chatList->setSelectionMode(QAbstractItemView::SingleSelection);
    ChatListItemDelegate *delegate = new ChatListItemDelegate(this);
    m_chatList->setItemDelegate(delegate);
    m_chatList->setModel(m_chatModel);

    connect(m_chatList, &QListView::clicked, this, &LeftPanelWidget::onChatItemClicked);
    connect(delegate, &ChatListItemDelegate::deleteRequested, this, &LeftPanelWidget::onDeleteChatRequested);

    // Enable context menu
    m_chatList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_chatList, &QWidget::customContextMenuRequested, this, &LeftPanelWidget::onContextMenu);

    layout->addWidget(m_chatList, 1);

    // ---------------- Trash confirmation -------------------------
    m_trashConfirmWidget = new QWidget(this);
    m_trashConfirmWidget->setVisible(false);

    auto trashLayout = new QHBoxLayout(m_trashConfirmWidget);
    trashLayout->setContentsMargins(0, 0, 0, 0);

    m_trashYes = new QPushButton(QString::fromUtf8("✓"), m_trashConfirmWidget);
    m_trashYes->setObjectName("trashYes");

    m_trashNo = new QPushButton(QString::fromUtf8("✕"), m_trashConfirmWidget);
    m_trashNo->setObjectName("trashNo");

    trashLayout->addWidget(m_trashYes);
    trashLayout->addWidget(m_trashNo);

    layout->addWidget(m_trashConfirmWidget);

    connect(m_trashYes, &QPushButton::clicked, this, &LeftPanelWidget::onConfirmDelete);
    connect(m_trashNo, &QPushButton::clicked, this, &LeftPanelWidget::onCancelDelete);

    m_autoHideTimer = new QTimer(this);
    m_autoHideTimer->setSingleShot(true);
    m_autoHideTimer->setInterval(3000);
    connect(m_autoHideTimer, &QTimer::timeout, this, &LeftPanelWidget::onCancelDelete);

    // ---------------- Updates ------------------------------------
    m_updatesButton = new QPushButton(tr("Updates"), this);
    m_updatesButton->setMinimumHeight(48);
    connect(m_updatesButton, &QPushButton::clicked, this, &LeftPanelWidget::onUpdates);
    layout->addWidget(m_updatesButton);

    // ---------------- Downloads ----------------------------------
    m_downloadsButton = new QPushButton(tr("Downloads"), this);
    m_downloadsButton->setMinimumHeight(48);
    connect(m_downloadsButton, &QPushButton::clicked, this, &LeftPanelWidget::downloadClicked);
    layout->addWidget(m_downloadsButton);

    // ---------------- About --------------------------------------
    m_aboutButton = new QPushButton(tr("About"), this);
    m_aboutButton->setMinimumHeight(48);
    connect(m_aboutButton, &QPushButton::clicked, this, &LeftPanelWidget::aboutClicked);
    layout->addWidget(m_aboutButton);
}

void LeftPanelWidget::createChatWidget(const QString &name)
{
    // find default connection
    if (!m_connection.isValid()) {
        foreach (auto connection, m_llmModel->getAllConnections()) {
            if (connection.isDefault()) {
                m_connection = connection;
                break;
            }
        }
    }

    if (!m_connection.isValid()) {
        LLMConnectionSelection dlg(m_llmModel, this);
        if (dlg.exec() != QDialog::DialogCode::Accepted) {
            return;
        }
        QString name = dlg.selectedConnectionName();
        foreach (auto connection, m_llmModel->getAllConnections()) {
            if (connection.name() == name) {
                m_connection = connection;
                break;
            }
        }
        if (!m_connection.isValid()) {
            return;
        }
    }

    if (MainWindow *mw = MainWindow::window()) {
        ChatPanelWidget *newWidget = new ChatPanelWidget( //
            &m_connection,
            mw->syntaxModel(),
            mw->toolModel(),
            mw->contentWidget());
        // Create initial chat
        m_chatModel->addChat(name, newWidget);
    }
}

void LeftPanelWidget::onChatItemClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    // Get the chat widget from the model
    ChatListModel::ChatData *chatData = m_chatModel->getChatData(index.row());
    if (chatData) {
        emit chatSelected(chatData->widget);
    }
}

void LeftPanelWidget::onEditChat()
{
    auto currentIndex = m_chatList->currentIndex();
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
        m_chatModel->setData(currentIndex, newName, Qt::EditRole);
    }
}

void LeftPanelWidget::onDeleteChat()
{
    auto currentIndex = m_chatList->currentIndex();
    if (!currentIndex.isValid())
        return;

    m_pendingDeleteIndex = currentIndex.row();
    m_trashConfirmWidget->setVisible(true);
    m_autoHideTimer->start();
}

void LeftPanelWidget::onDeleteChatRequested(int row)
{
    m_pendingDeleteIndex = row;
    m_trashConfirmWidget->setVisible(true);
    m_autoHideTimer->start();
}

void LeftPanelWidget::onConfirmDelete()
{
    if (m_pendingDeleteIndex >= 0 && m_pendingDeleteIndex < m_chatModel->chatCount()) {
        m_chatModel->removeChat(m_pendingDeleteIndex);
    }

    m_pendingDeleteIndex = -1;
    m_trashConfirmWidget->setVisible(false);
}

void LeftPanelWidget::onCancelDelete()
{
    m_pendingDeleteIndex = -1;
    m_trashConfirmWidget->setVisible(false);
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
    int count = m_chatModel->chatCount();
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
    auto currentIndex = m_chatList->currentIndex();
    editAction->setEnabled(currentIndex.isValid());
    deleteAction->setEnabled(currentIndex.isValid());

    // Show the context menu
    QAction *selectedAction = contextMenu.exec(m_chatList->mapToGlobal(point));

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
