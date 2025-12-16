#include <attachbutton.h>
#include <chatpanelwidget.h>
#include <chattextwidget.h>
#include <filelistmodel.h>
#include <filelistwidget.h>
#include <filenamelabel.h>
#include <llmconnectionmodel.h>
#include <mainwindow.h>
#include <progresspopup.h>
#include <settingsmanager.h>
#include <toolservice.h>
#include <toolswidget.h>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QStandardPaths>
#include <QStyle>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>

ChatPanelWidget::ChatPanelWidget(LLMConnection *connection, SyntaxColorModel *scModel, ToolModel *tModel, QWidget *parent)
    : QWidget(parent)
    , m_llmConnection(connection)
    , m_llmclient(new LLMChatClient(tModel, new ChatModel(this), this))
    , m_syntaxModel(scModel)
    , m_toolModel(tModel)
    , m_fileListModel(new FileListModel(this))
    , m_isConversating(false)
{
    // fixed layout height
    setMinimumHeight(640);

    // create extension to language mapping once
    m_syntaxModel->loadSyntaxModel();

    // Widget main area
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(12);

    // Scrollable chat area  with chat text view
    QWidget *messageContainer = createChatArea(this);
    mainLayout->addWidget(messageContainer, Qt::AlignTop);

    // Input area with hidden file list and text input field
    QWidget *inputContainer = createInputArea(this);
    mainLayout->addWidget(inputContainer);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    splitter->setHandleWidth(10);
    splitter->insertWidget(0, messageContainer);
    splitter->insertWidget(1, inputContainer);
    splitter->setSizes(QList<int>() << 700 << 100);
    mainLayout->addWidget(splitter);

    // Language Model Selection Widget
    QWidget *modelWidget = createLLMSelector(this);
    mainLayout->addWidget(modelWidget);

    // Buttons row
    QWidget *buttonBox = createButtonBox(this);
    mainLayout->addWidget(buttonBox);

    // assign layout to widget
    setLayout(mainLayout);

    // Connect chat message model events
    connectChatModel();

    // Connect LLM client events
    connectLLMClient();

    // Load splitter position
    SettingsManager *settings = MainWindow::window()->settings();
    settings->loadSplitterPosition("chat", splitter, 800, 100);

    connect(splitter, &QSplitter::splitterMoved, this, [settings, splitter](int, int) { //
        settings->saveSplitterPosition("chat", splitter);
    });

    // Connect LLM server
    if (m_llmConnection && m_llmConnection->isValid()) {
        QTimer::singleShot(10, this, [this]() {
            m_llmclient->setConnection(m_llmConnection);
            m_llmclient->listModels();
        });
    }
}

// ----------------- chat text area -----------------

inline QWidget *ChatPanelWidget::createChatArea(QWidget *parent)
{
    QScrollArea *scrollArea;
    scrollArea = new QScrollArea(parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *container;
    container = new QWidget(scrollArea);
    container->setObjectName("messagesContainer");
    container->setSizePolicy( //
        QSizePolicy::Policy::Expanding,
        QSizePolicy::Policy::Expanding);

    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setAlignment(Qt::AlignTop);

    container->setLayout(layout);
    scrollArea->setWidget(container);

    m_chatView = new ChatTextWidget(container, m_syntaxModel);
    m_chatView->setSizePolicy( //
        QSizePolicy::Policy::Expanding,
        QSizePolicy::Policy::Expanding);
    layout->addWidget(m_chatView);

    connect(m_chatView, &ChatTextWidget::documentUpdated, this, [this]() { //
        onHideProgressPopup();
    });

    return scrollArea;
}

inline QWidget *ChatPanelWidget::createInputArea(QWidget *parent)
{
    QWidget *inputContainer = new QWidget(parent);
    inputContainer->setSizePolicy( //
        QSizePolicy::Expanding,
        QSizePolicy::MinimumExpanding);

    QVBoxLayout *inputLayout = new QVBoxLayout(inputContainer);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(12);

    // File list widget (initially hidden)
    QWidget *fileListWidget = createFileListWidget(inputContainer);
    inputLayout->addWidget(fileListWidget);

    // Multiline input field
    QWidget *inputWidget = createInputWidget(inputContainer);
    inputLayout->addWidget(inputWidget);
    inputContainer->setLayout(inputLayout);
    return inputContainer;
}

inline QWidget *ChatPanelWidget::createFileListWidget(QWidget *parent)
{
    FileListWidget *widget;
    widget = new FileListWidget(m_fileListModel, parent);
    widget->setObjectName("fileListWidget");
    widget->setSizePolicy( //
        QSizePolicy::Expanding,
        QSizePolicy::Minimum);
    //m_fileListWidget->setMaximumHeight(100);
    widget->setModel(m_fileListModel);
    widget->setVisible(false);

    return widget;
}

inline QWidget *ChatPanelWidget::createInputWidget(QWidget *parent)
{
    m_messageInput = new QTextEdit(parent);
#ifdef Q_OS_MACOS
    m_messageInput->setPlaceholderText(tr("Type your message... | Option+Enter (⌥ + ⏎) submit."));
#else
    messageInput->setPlaceholderText(tr("Type your message... | Alt+Enter submit."));
#endif
    m_messageInput->setAcceptRichText(false);
    m_messageInput->setAutoFormatting(QTextEdit::AutoAll);
    // Enable drag and drop for message input
    m_messageInput->setAcceptDrops(true);
    m_messageInput->setSizePolicy( //
        QSizePolicy::Policy::Expanding,
        QSizePolicy::Policy::MinimumExpanding);

    connect(m_messageInput, &QTextEdit::textChanged, this, [this]() { //
        QTextDocument *doc = m_messageInput->document();
        //m_sendButton->setEnabled(doc->characterCount() > 0);
        m_sendButton->setEnabled(doc->isModified());

    });

    return m_messageInput;
}

// Language model selection widget
inline QWidget *ChatPanelWidget::createLLMSelector(QWidget *parent)
{
    QWidget *modelWidget;
    QComboBox *comboBox;
    QLabel *modelLabel;

    modelWidget = new QWidget(parent);
    modelWidget->setObjectName("modelWidget");
    modelWidget->setSizePolicy( //
        QSizePolicy::Expanding,
        QSizePolicy::Minimum);

    QHBoxLayout *modelLayout = new QHBoxLayout(modelWidget);
    modelLayout->setContentsMargins(12, 12, 12, 12);

    modelLabel = new QLabel(tr("Select Model:"), modelWidget);
    modelLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    comboBox = new QComboBox(modelWidget);
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(comboBox, &QComboBox::currentIndexChanged, this, [this, comboBox](int index) {
        if (index < 0 || index >= comboBox->count())
            return;
        QVariant data = comboBox->itemData(index);
        if (data.canConvert<ModelListModel::ModelEntry>()) {
            m_llmclient->setActiveModel(data.value<ModelListModel::ModelEntry>());
        }
    });

    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(comboBox);

    // Connect LLM list model
    connect(m_llmclient->llmModels(), &ModelListModel::modelsLoaded, this, [this, comboBox]() {
        QTimer::singleShot(10, this, [this, comboBox]() {
            const QList<ModelListModel::ModelEntry> &models = m_llmclient->llmModels()->modelList();
            // Fill Model selection QComboBox
            comboBox->clear();
            for (const ModelListModel::ModelEntry &entry : models) {
                comboBox->addItem(entry.id, QVariant::fromValue(entry));
            }
            // Select first model by default
            if (comboBox->count() > 0) {
                comboBox->setCurrentIndex(0);
                // Save selected model entry
                QVariant data = comboBox->itemData(0);
                if (data.canConvert<ModelListModel::ModelEntry>()) {
                    m_llmclient->setActiveModel(data.value<ModelListModel::ModelEntry>());
                }
            }
        });
    });

    return modelWidget;
}

inline QWidget *ChatPanelWidget::createButtonBox(QWidget *parent)
{
    QWidget *container = new QWidget(parent);
    container->setSizePolicy( //
        QSizePolicy::Expanding,
        QSizePolicy::Minimum);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(12);
    // Align layout to the right
    buttonLayout->setAlignment(Qt::AlignRight);
    // First add spacer to push buttons to the right
    buttonLayout->addStretch();
    buttonLayout->addWidget(createAttachButton(container));
    buttonLayout->addWidget(createToolsButton(container));
    buttonLayout->addWidget(createSendButton(container));

    container->setLayout(buttonLayout);
    return container;
}

inline AttachButton *ChatPanelWidget::createAttachButton(QWidget *parent)
{
    m_attachButton = new AttachButton(tr("Attach"), parent);

    connect(m_attachButton, &AttachButton::clicked, this, [this]() {
        QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        if (m_lastDirectory.isEmpty())
            m_lastDirectory = homePath;
        QString file = QFileDialog::getOpenFileName( //
            this,
            "Select File to Attach",
            m_lastDirectory,
            "Source Files (*.cpp *.c *.h *.hpp *.cc *.cxx *.hxx *.cs *.java *.py *.js *.ts *.php *.rb *.pl *.sh *.go *.rs *.swift *.kt *.scala *.r *.m *.mm);;"
            "Text Files (*.txt *.md *.log);;"
            "Configuration Files (*.json *.xml *.yaml *.yml *.ini *.cfg);;"
            "All Files (*)");
        if (file.isEmpty())
            return;
        // remember path
        QFileInfo fi(file);
        m_lastDirectory = fi.filePath();
        // Add file to the file list model
        m_fileListModel->addFile(file, style()->standardIcon(QStyle::SP_FileIcon));
    });
    connect(m_attachButton, &AttachButton::fileDropped, this, [this](const QList<QUrl> &urls) {
        const QIcon icon = style()->standardIcon(QStyle::SP_FileIcon);
        foreach (const QUrl &url, urls) {
            QString filePath = url.toLocalFile();
            if (!filePath.isEmpty()) {
                // Add to file list model
                m_fileListModel->addFile(filePath, icon);
            }
        }
    });
    return m_attachButton;
}

inline QPushButton *ChatPanelWidget::createToolsButton(QWidget *parent)
{
    QPushButton *toolsButton = new QPushButton(tr("Tools"), parent);
    toolsButton->setEnabled(m_toolModel->rowCount() > 0);

    connect(toolsButton, &QPushButton::clicked, this, [this]() {
        QMainWindow *toolsWindow = nullptr;
        QVariant v = sender()->property("window");
        if (!v.isNull() && v.isValid()) {
            toolsWindow = v.value<QMainWindow *>();
        }
        // Create a new window for the tools
        if (toolsWindow == nullptr) {
            toolsWindow = new QMainWindow(this->window());
            toolsWindow->setWindowFlag(Qt::WindowType::Tool, true);
            toolsWindow->setWindowTitle(qApp->applicationDisplayName() + " - " + tr("Tools"));
            toolsWindow->setWindowIcon(QIcon(":/assets/eofaichat.png"));
            // Set window properties
            toolsWindow->setFixedSize(380, 420);
            toolsWindow->resize(toolsWindow->size());
            // Create the ToolsWidget
            ToolsWidget *toolsWidget = new ToolsWidget(m_toolModel, toolsWindow);
            // Set the ToolsWidget as the central widget of the window
            toolsWindow->setCentralWidget(toolsWidget);
            sender()->setProperty("window", QVariant::fromValue(toolsWindow));
        }
        // Show the window
        toolsWindow->setWindowState(Qt::WindowState::WindowActive);
        toolsWindow->show();
    });

    // Tools, Resource, Prompts configuration
    connect(m_toolModel, &ToolModel::toolAdded, this, [this, toolsButton](const ToolModel::ToolModelEntry &entry) { //
        qDebug("Tool config type %d added: %s", entry.type, qPrintable(entry.name));
        toolsButton->setEnabled(m_toolModel->hasExecutables()  //
                                || m_toolModel->hasResources() //
                                || m_toolModel->hasPrompts());
    });
    connect(m_toolModel, &ToolModel::toolRemoved, this, [this, toolsButton](int) { //
        toolsButton->setEnabled(m_toolModel->hasExecutables()                      //
                                || m_toolModel->hasResources()                     //
                                || m_toolModel->hasPrompts());
    });

    m_toolModel->loadToolsConfig();

    return toolsButton;
}

inline QPushButton *ChatPanelWidget::createSendButton(QWidget *parent)
{
    m_sendButton = new QPushButton(tr("Send"), parent);
    m_sendButton->setEnabled(false);

    connect(m_sendButton, &QPushButton::clicked, this, [this]() {
        // check in conversation
        if (m_isConversating) {
            m_llmclient->cancelRequest();
            return;
        }

        QList<LLMChatClient::SendParameters> messages;
#if 1
        /*
        QByteArray question = m_messageInput->toPlainText().trimmed().toUtf8();
        if (question.isEmpty())
            return;

        // New question
        messages.append({
            .role = ChatMessage::Role::UserRole,
            .message = question,
        });
        */
        // Chat history
        for (int i = 0; i < m_llmclient->chatModel()->rowCount(); i++) {
            ChatMessage *cm = m_llmclient->chatModel()->messageAt(i);
            if (cm->isUser()) {
                messages.append({
                    .role = cm->role(),
                    .content = cm->content(),
                });
            } else {
                foreach (auto t, cm->tools()) {
                    messages.append({
                        .role = cm->role(),
                        .toolName = t.functionName(),
                        .toolQuery = t.functionName(),
                        .toolResult = cm->toolContent(),
                    });
                }
            }
        }

        // new user question
        QStringList text;
        QByteArray question = m_messageInput->toPlainText().trimmed().toUtf8();
        if (question.isEmpty())
            return;
        text.append(question);
        // load file attachments
        if (m_fileListModel->rowCount() > 0) {
            QByteArray content;
            for (int i = 0; i < m_fileListModel->rowCount(); i++) {
                if (FileItem *_item = dynamic_cast<FileItem *>(m_fileListModel->item(i))) {
                    text.append(QStringLiteral("#File name: %1").arg(_item->fileInfo().absoluteFilePath()));
                    text.append(m_fileListModel->readFileContent(i));
                }
            }
        }
        messages.append({
            .role = ChatMessage::Role::UserRole,
            .content = text.join("\n"),
        });

#else
        QByteArray question = m_messageInput->toPlainText().trimmed().toUtf8();
        if (question.isEmpty())
            return;

        // app new conversation
        QByteArray text;
        text.append("<|im_start|>user\n");
        text.append(question);
        text.append("<|im_end|>\n");

        // load previous chat messages
        QByteArray previusChat = m_llmclient->chatModel()->chatContent();
        if (previusChat.length() > 0) {
            text.append(previusChat);
        }

        // load file attachments
        if (m_fileListModel->rowCount() > 0) {
            QByteArray content;
            text.append("#Attached files:\n");
            m_fileListModel->loadContentOfFiles(content);
            if (!content.isEmpty()) {
                text.append("<|im_start|>user\n");
                text = text.append(content);
                text.append("<|im_end|>\n");
            }
        }
#endif

        // Add sender bubble
        ChatMessage cm(m_llmclient->chatModel());
        cm.setContent(question);
        cm.setRole(ChatMessage::Role::ChatRole);
        cm.setId(QStringLiteral("CPW-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        cm.setSystemFingerprint(cm.id());
        cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
        cm.setModel(m_llmclient->activeModel().id);
        m_llmclient->chatModel()->addMessage(cm);

        // Clear input
        m_messageInput->clear();
        m_fileListModel->clear();

        // Show progress popup
        // onShowProgressPopup();
        m_sendButton->setText(tr("Stop"));
        m_messageInput->setEnabled(false);
        m_attachButton->setEnabled(false);

        m_isConversating = true;

        // Send JSON for network layer
        //m_llmclient->sendChat(cm.model(), text, true);
        m_llmclient->sendChat(messages, true);
    });

    return m_sendButton;
}

// ==================================================

void ChatPanelWidget::onConnectionChanged(LLMConnection *connection)
{
    m_llmConnection = connection;
    if (m_llmConnection && m_llmConnection->isValid()) {
        QTimer::singleShot(10, this, [this]() {
            m_llmclient->setConnection(m_llmConnection);
            m_llmclient->listModels();
        });
    }
}

void ChatPanelWidget::onUpdateChatText(int index, ChatMessage *message)
{
    qDebug().noquote() << "[ChatPanelWidget] onUpdateChatText index:" << index //
                       << "id:" << message->id();
    // ensure UI thread
    QTimer::singleShot(10, this, [index, message, this]() {
        if (index < 0) {
            m_chatView->appendMessage(message);
        } else {
            m_chatView->updateMessage(message);
        }
    });
}

inline void ChatPanelWidget::connectChatModel()
{
    connect(m_llmclient->chatModel(), &ChatModel::messageAdded, this, [this](ChatMessage *message) { //
        onUpdateChatText(-1, message);
    });
    connect(m_llmclient->chatModel(), &ChatModel::messageChanged, this, [this](int index, ChatMessage *message) { //
        onUpdateChatText(index, message);
    });
    connect(m_llmclient->chatModel(), &ChatModel::messageRemoved, this, [](int) { //
        //
    });
}

inline void ChatPanelWidget::reportLLMError(QNetworkReply::NetworkError error, const QString &message)
{
    qCritical().noquote() << "[ChatPanelWidget]" << message << error;
    ChatMessage cm(m_llmclient->chatModel());
    cm.setRole(ChatMessage::Role::AssistantRole);
    cm.setId(QStringLiteral("CPW-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    cm.setSystemFingerprint(cm.id());
    cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
    cm.setModel(m_llmclient->activeModel().id);
    cm.setContent(QStringLiteral("Error: %1\n%2").arg(error).arg(message));

    m_llmclient->chatModel()->addMessage(cm);
    m_llmclient->cancelRequest();

    QTimer::singleShot(10, this, [this]() { //
        onHideProgressPopup();
    });
}

inline void ChatPanelWidget::connectLLMClient()
{
    connect(m_llmclient, &LLMChatClient::toolRequest, this, &ChatPanelWidget::onToolRequest, Qt::QueuedConnection);
    connect(m_llmclient, &LLMChatClient::streamCompleted, this, &ChatPanelWidget::onHideProgressPopup, Qt::QueuedConnection);
    connect(m_llmclient, &LLMChatClient::networkError, this, [this](QNetworkReply::NetworkError error, const QString &message) { //
        reportLLMError(error, message);
    });
    connect(m_llmclient, &LLMChatClient::errorOccurred, this, [this](const QString &message) { //
        reportLLMError(QNetworkReply::NetworkError::OperationCanceledError, message);
    });
}

// ---------------- Progress Popup Methods ----------------

void ChatPanelWidget::onShowProgressPopup()
{
    if (m_progressPopup)
        m_progressPopup->deleteLater();
    // Initialize progress popup
    m_progressPopup = new ProgressPopup(this);
    // Enable blur effect on central widget
    m_progressPopup->setBlurEffect(true);
    // Show centered popup
    m_progressPopup->showCentered();
}

void ChatPanelWidget::onHideProgressPopup()
{
    // Disable blur effect
    if (m_progressPopup) {
        m_progressPopup->setBlurEffect(false);
        m_progressPopup->hide();
        m_progressPopup->deleteLater();
        m_progressPopup = nullptr;
    }

    m_sendButton->setText(tr("Send"));
    m_sendButton->setEnabled(true);
    m_messageInput->setEnabled(true);
    m_attachButton->setEnabled(true);
    m_isConversating = false;
}

// ---------------- Drag and Drop Events ----------------

void ChatPanelWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void ChatPanelWidget::dropEvent(QDropEvent *event)
{
    const QIcon icon = style()->standardIcon(QStyle::SP_FileIcon);

    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        foreach (const QUrl &url, urls) {
            QString filePath = url.toLocalFile();
            if (!filePath.isEmpty()) {
                // Check if mouse is over messageInput
                QPoint pos = event->position().toPoint();
                QPoint messageInputPos = m_messageInput->mapTo(this, QPoint(0, 0));
                QRect messageInputRect(messageInputPos, m_messageInput->size());
                if (messageInputRect.contains(pos)) {
                    // Drop on message input - insert full path
                    QString text = m_messageInput->toPlainText();
                    m_messageInput->setText(text + filePath);
                } else {
                    // Drop on main widget - add to file list
                    m_fileListModel->addFile(filePath, icon);
                }
            }
        }
        event->acceptProposedAction();
    } else if (event->mimeData()->hasText()) {
        // Handle text drop (e.g., file paths)
        QString text = event->mimeData()->text();
        if (!text.isEmpty()) {
            // Check if mouse is over messageInput
            QPoint pos = event->position().toPoint();
            QPoint messageInputPos = m_messageInput->mapTo(this, QPoint(0, 0));
            QRect messageInputRect(messageInputPos, m_messageInput->size());
            if (messageInputRect.contains(pos)) {
                // Drop on message input - insert full path
                QString currentText = m_messageInput->toPlainText();
                m_messageInput->setText(currentText + text);
            } else {
                // Drop on main widget - add to file list
                m_fileListModel->addFile(text, icon);
            }
        }
        event->acceptProposedAction();
    }
}

// ---------------- Key Press Event ----------------

void ChatPanelWidget::keyPressEvent(QKeyEvent *event)
{
    // Check if Ctrl+Enter or Cmd+Enter is pressed (depending on platform)
    bool isCtrlPressed = event->modifiers().testFlag(Qt::ControlModifier);
    bool isCmdPressed = event->modifiers().testFlag(Qt::MetaModifier); // For macOS
    bool isShiftPressed = event->modifiers().testFlag(Qt::ShiftModifier);

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // If Shift is pressed, create a new line
        if (isShiftPressed) {
            // Allow the default behavior (new line)
            QWidget::keyPressEvent(event);
            return;
        }

        // If Ctrl+Enter or Cmd+Enter is pressed, create a new line
        if (isCtrlPressed || isCmdPressed) {
            // Allow the default behavior (new line)
            QWidget::keyPressEvent(event);
            return;
        }

        // If Enter is pressed without modifiers, send the message
        // Call the send button's click handler directly
        if (m_sendButton) {
            m_sendButton->click();
        }
        return;
    }

    // For all other key events, call the parent implementation
    QWidget::keyPressEvent(event);
}

// ---------------- Chat Tooling Events ----------------

void ChatPanelWidget::onToolRequest(ChatMessage *message, const ChatMessage::ToolEntry &toolCall)
{
    ToolService toolService(this);
    ToolModel::ToolModelEntry tool;
    QJsonObject toolResult;
    QJsonObject content;
    QByteArray buffer;

    qDebug().noquote() << "[ChatPanelWidget] onToolRequest type:" //
                       << toolCall.toolType()                     //
                       << "id:" << toolCall.toolCallId()          //
                       << "function:" << toolCall.functionName()  //
                       << "args:" << toolCall.arguments();

    if (toolCall.functionName().isEmpty()) {
        toolResult = toolService.createErrorResponse( //
            QStringLiteral("The function name is required."));
        goto finish;
    }

    tool = m_toolModel->toolByName(toolCall.functionName());
    if (tool.name.isEmpty() || tool.type == ToolModel::ToolModelType::ToolUnknown) {
        toolResult = toolService.createErrorResponse( //
            QStringLiteral("Unable to find function: %1").arg(toolCall.functionName()));
        goto finish;
    }

    // ---------------------

    switch (toolCall.toolType()) {
        case ChatMessage::ToolType::Function: {
            toolResult = toolService.execute(tool, toolCall.arguments());
            break;
        }
        case ChatMessage::ToolType::Resuource: {
            toolResult = toolService.execute(tool, toolCall.arguments());
            break;
        }
        case ChatMessage::ToolType::Prompt: {
            toolResult = toolService.execute(tool, toolCall.arguments());
            break;
        }
        default: {
            toolResult = toolService.createErrorResponse( //
                QStringLiteral("Invalid tool type in function: %1").arg(toolCall.functionName()));
            goto finish;
        }
    }

    // ---------------------

    if (toolResult.isEmpty()) {
        content = toolService.createErrorResponse( //
            QStringLiteral("Tool '%1' does not produce any results.").arg(toolCall.functionName()));
    } else if (toolResult.contains("structuredContent")) {
        content = toolResult["structuredContent"].toObject();
    } else if (toolResult.contains("content")) {
        QJsonValue contentValue = toolResult["content"];
        if (contentValue.isArray()) {
            QJsonArray items = contentValue.toArray();
            if (!items.isEmpty() && items[0].isObject()) {
                QJsonObject itemObject = items[0].toObject();
                if (!itemObject.contains("type")) {
                    content = toolService.createErrorResponse( //
                        QStringLiteral("Tool '%1' Response invalid. Field 'type' missed.").arg(toolCall.functionName()));
                    goto finish;
                }
                QString type = itemObject["type"].toString("text");
                if (type.toLower().trimmed() == "object") {
                    content = itemObject;
                } else if (type.toLower().trimmed() == "text") {
                    if (itemObject["text"].isNull() || !itemObject["text"].isString()) {
                        //buffer = "\n<|im_start|>assistant\nFailed to execute tool.\n<|im_end|>\n<|im_stop|>\n";
                        buffer = "Failed to execute tool.\n<|im_stop|>\n";
                    } else {
                        //buffer = QStringLiteral("\n<|im_start|>assistant\n%1\n<|im_end|>\n<|im_stop|>\n").arg(itemObject["text"].toString()).toUtf8();
                        buffer = QStringLiteral("%1\n<|im_stop|>\n").arg(itemObject["text"].toString()).toUtf8();
                    }
                }
            } else {
                content = {
                    QPair<QString, QJsonValue>("items", QJsonArray({""})),
                };
            }
        } else {
            content = contentValue.toObject();
        }
    } else {
        content = toolResult;
    }

finish:
    if (buffer.isEmpty()) {
        content["stop"] = true;
        //buffer.append("<|im_start|>assistant\n");
        buffer.append(QJsonDocument(content).toJson(QJsonDocument::Indented));
        //buffer.append("\n<|im_end|>\n");
    }

    QString errmsg;
    if (toolResult.contains("success") && !toolResult["success"].toBool()) {
        errmsg = "\n" + buffer + "\n";
    }

    ChatMessage cm(m_llmclient->chatModel());
    cm.setRole(ChatMessage::Role::ToolingRole);
    cm.setId(QStringLiteral("CPW-%1").arg(message->id()));
    cm.setSystemFingerprint(cm.id());
    cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
    cm.setModel(m_llmclient->activeModel().id);
    cm.setToolContent(buffer);

    if (errmsg.length() > 0) {
        cm.setContent(tr("Tool(%1:%2:%3) call failed.%4") //
                          .arg(toolCall.toolType())
                          .arg(toolCall.toolCallId(), //
                               toolCall.functionName(),
                               errmsg));
    } else {
        cm.setContent(tr("Tool(%1:%2:%3) call completed.") //
                          .arg(toolCall.toolType())
                          .arg(toolCall.toolCallId(), //
                               toolCall.functionName()));
    }

    m_llmclient->chatModel()->addMessage(cm);

    // Send message to LLM
    LLMChatClient::SendParameters params = {
        .role = ChatMessage::ToolingRole,
        .toolName = tool.name,
        .toolQuery = tool.title + "(" + tool.description + ")",
        .toolResult = buffer,
    };
    m_llmclient->sendChat(params, true);
}
