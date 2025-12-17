#include <chatlistitemdelegate.h>
#include <chatpanelwidget.h>
#include <leftpanelwidget.h>
#include <llmconnectionselection.h>
#include <mainwindow.h>
#include <QApplication>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStyle>

LeftPanelWidget::LeftPanelWidget(QWidget *parent)
    : QWidget(parent)
    , m_chatListModel(new ChatListModel(this))
    , m_llmModel(MainWindow::window()->llmConnections())
{
    setMinimumWidth(150);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(12);

    // ---------------- New Chat ----------------------------------
    m_newChatButton = new QPushButton(tr("New LLM Chat"), this);
    m_newChatButton->setMinimumHeight(48);
    connect(m_newChatButton, &QPushButton::clicked, this, &LeftPanelWidget::onNewChatClicked);
    layout->addWidget(m_newChatButton);

    // ---------------- Chat List ----------------------------------

    // connect new chat with connection changed and select as active chat
    connect(m_chatListModel, &ChatListModel::chatWidgetAdded, this, [this](QWidget *w) {
        if (ChatPanelWidget *cpw = qobject_cast<ChatPanelWidget *>(w)) {
            // notify about LL model server connection changed
            connect(this, &LeftPanelWidget::connectionSelected, cpw, &ChatPanelWidget::onConnectionChanged);
            // save chat messages with active chat name
            connect(cpw, &ChatPanelWidget::chatTextUpdated, this, &LeftPanelWidget::onSaveChatHistory);
        }
        // Select the new chat
        if (m_chatListModel->rowCount() > 0) {
            QModelIndex newIndex = m_chatListModel->index(m_chatListModel->rowCount() - 1, 0);
            m_chatListView->setCurrentIndex(newIndex);
        }
        // notify main window to insert the widget into layout
        emit chatSelected(w);
    });

    connect(m_chatListModel, &ChatListModel::chatWidgetRemoved, this, [this](QWidget *w) { //
        // notify main window to remove the widget from layout
        emit chatRemoved(w);
    });

    m_chatListView = new QListView(this);
    m_chatListView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_chatListView->setModel(m_chatListModel);
    m_chatListView->setEditTriggers(QListView::NoEditTriggers);
    m_chatListView->setCurrentIndex(QModelIndex());

    // active selected chat
    connect(m_chatListView, &QListView::clicked, this, &LeftPanelWidget::onChatItemClicked);
    // Edit chat name dialog
    connect(m_chatListView, &QListView::doubleClicked, this, &LeftPanelWidget::onChatItemDoubleClicked);

    ChatListItemDelegate *delegate = new ChatListItemDelegate(this);
    connect(delegate, &ChatListItemDelegate::deleteRequested, this, &LeftPanelWidget::onDeleteChatRequested);
    m_chatListView->setItemDelegate(delegate);

    // Enable context menu
    m_chatListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_chatListView, &QWidget::customContextMenuRequested, this, &LeftPanelWidget::onContextMenu);

    layout->addWidget(m_chatListView, 1);

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
    m_updatesButton = new QPushButton(tr("Connection"), this);
    m_updatesButton->setMinimumHeight(48);
    connect(m_updatesButton, &QPushButton::clicked, this, &LeftPanelWidget::onSelectConnection);
    layout->addWidget(m_updatesButton);
#if 0
    // ---------------- Downloads ----------------------------------
    m_downloadsButton = new QPushButton(tr("Downloads"), this);
    m_downloadsButton->setMinimumHeight(48);
    connect(m_downloadsButton, &QPushButton::clicked, this, &LeftPanelWidget::downloadClicked);
    layout->addWidget(m_downloadsButton);
#endif
    // ---------------- About --------------------------------------
    m_aboutButton = new QPushButton(tr("About"), this);
    m_aboutButton->setMinimumHeight(48);
    connect(m_aboutButton, &QPushButton::clicked, this, &LeftPanelWidget::aboutClicked);
    layout->addWidget(m_aboutButton);
}

void LeftPanelWidget::onChatItemClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    // Get the chat widget from the model
    if (ChatListModel::ChatData *chatData = m_chatListModel->chatData(index.row())) {
        // notify main window to insert the widget into layout
        emit chatSelected(chatData->widget);
    }
}

void LeftPanelWidget::onChatItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    // Call the edit chat function when double clicked
    onEditChatClicked();
}

void LeftPanelWidget::onEditChatClicked()
{
    auto currentIndex = m_chatListView->currentIndex();
    if (!currentIndex.isValid())
        return;

    QString currentName = currentIndex.data(Qt::DisplayRole).toString();

    bool ok = false;
    QString newName = QInputDialog::getText(this, tr("Rename chat"), tr("Enter name:"), QLineEdit::Normal, currentName, &ok);
    if (ok) {
        m_chatListModel->setData(currentIndex, newName, Qt::EditRole);
    }
}

void LeftPanelWidget::onDeleteChatClicked()
{
    auto currentIndex = m_chatListView->currentIndex();
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
    if (m_pendingDeleteIndex >= 0 && m_pendingDeleteIndex < m_chatListModel->chatCount()) {
        m_chatListModel->removeChat(m_pendingDeleteIndex);
    }

    m_pendingDeleteIndex = -1;
    m_trashConfirmWidget->setVisible(false);
}

void LeftPanelWidget::onCancelDelete()
{
    m_pendingDeleteIndex = -1;
    m_trashConfirmWidget->setVisible(false);
}

void LeftPanelWidget::onSelectConnection()
{
    LLMConnectionSelection dlg(m_llmModel, this);
    if (dlg.exec() != QDialog::DialogCode::Accepted) {
        return;
    }
    QString name = dlg.selectedConnectionName();
    foreach (auto connection, m_llmModel->getAllConnections()) {
        if (connection.name() == name) {
            m_llmModel->setDefaultConnection(name);
            m_connection = connection;
            break;
        }
    }
    if (m_connection.isValid()) {
        emit connectionSelected(&m_connection);
    }
}

void LeftPanelWidget::onChatNameChanged(const QString & /*newName*/)
{
    // This slot can be used to update the chat name in the model
    // when it's changed from outside
}

void LeftPanelWidget::onNewChatClicked()
{
    QString name = tr("New LLM chat");
    int count = m_chatListModel->chatCount();
    if (count > 0) {
        name = QString(tr("New LLM chat %1")).arg(count + 1);
    }

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
        onSelectConnection();
    }

    if (!m_connection.isValid()) {
        QMessageBox::information(this, qApp->applicationDisplayName(), "Invalid connection detected.");
        return;
    }

    if (MainWindow *mw = MainWindow::window()) {
        ChatPanelWidget *newWidget = new ChatPanelWidget( //
            &m_connection,                                // to selected or default connection
            mw->syntaxModel(),                            // global syntax model
            mw->toolModel(),                              // global supported tools
            mw->contentWidget());                         // placed in container widget
        // Create initial chat (raised chatWidgetAdded)
        m_chatListModel->addChat(name, newWidget);
    }
}

void LeftPanelWidget::onContextMenu(const QPoint &point)
{
    // Create context menu
    QMenu contextMenu(this);

    // Add actions to the context menu
    QAction *addAction = contextMenu.addAction(tr("Add Chat"));
    QAction *editAction = contextMenu.addAction(tr("Edit Chat"));
    QAction *deleteAction = contextMenu.addAction(tr("Delete Chat"));
    // --
    contextMenu.addSeparator();
    // Add action to save chat history
    QAction *saveAction = contextMenu.addAction(tr("Save Chat History"));

    // Enable/disable actions based on selection
    auto currentIndex = m_chatListView->currentIndex();
    editAction->setEnabled(currentIndex.isValid());
    deleteAction->setEnabled(currentIndex.isValid());
    saveAction->setEnabled(currentIndex.isValid());

    // Show the context menu
    QAction *selectedAction = contextMenu.exec(m_chatListView->mapToGlobal(point));

    if (selectedAction == addAction) {
        onNewChatClicked();
    } else if (selectedAction == editAction) {
        onEditChatClicked();
    } else if (selectedAction == deleteAction) {
        onDeleteChatClicked();
    } else if (selectedAction == saveAction) {
        // Get the chat name and save history
        onSaveChatHistory();
    }
}

void LeftPanelWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        // Handle delete/backspace key press
        onDeleteChatClicked();
    } else {
        // Call the parent implementation for other keys
        QWidget::keyPressEvent(event);
    }
}

// New slot to save chat history
void LeftPanelWidget::onSaveChatHistory()
{
    auto currentIndex = m_chatListView->currentIndex();
    QString chatName = currentIndex.data(Qt::DisplayRole).toString();

    // Find the index of the chat with the given name
    int row = -1;
    for (int i = 0; i < m_chatListModel->chatCount(); ++i) {
        QString name = m_chatListModel->data(m_chatListModel->index(i, 0), Qt::DisplayRole).toString();
        if (name == chatName) {
            row = i;
            break;
        }
    }

    // Chat not found, return
    if (row == -1) {
        return;
    }

    // Get the ChatModel from the ChatPanelWidget
    ChatModel *chatModel = m_chatListModel->chatModel(row);
    if (!chatModel) {
        // ChatModel not found, return
        return;
    }

    // Create a directory for chat histories if it doesn't exist
    QDir chatDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chat_histories");
    if (!chatDir.exists()) {
        QFile::Permissions permissions;
        permissions.setFlag(QFile::Permission::ReadOwner, true);
        permissions.setFlag(QFile::Permission::ReadGroup, true);
        permissions.setFlag(QFile::Permission::WriteOwner, true);
        permissions.setFlag(QFile::Permission::WriteGroup, true);
        permissions.setFlag(QFile::Permission::ExeOwner, true);
        permissions.setFlag(QFile::Permission::ExeGroup, true);
        if (!chatDir.mkpath(chatDir.absolutePath(), permissions)) {
            qWarning("Unable to create directory: %s", qPrintable(chatDir.absolutePath()));
            return;
        }
    }

    // Sanitize the chat name to create a valid filename
    QString sanitizedChatName = chatName;

    // Replace invalid characters with underscores
    sanitizedChatName.replace(QRegularExpression("[^a-zA-Z0-9_.-]"), "_");

    // Create the file path
    QString filePath = chatDir.absoluteFilePath(sanitizedChatName + ".json");

    // Save the chat history to the file
    chatModel->saveToFile(filePath);
}
