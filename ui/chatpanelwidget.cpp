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
    , m_chatModel(new ChatModel(this))
    , m_activeConnection(connection)
    , m_llmClient(new LLMChatClient(tModel, this))
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
    if (m_activeConnection && m_activeConnection->isValid()) {
        QTimer::singleShot(10, this, [this]() {
            m_llmClient->setConnection(m_activeConnection);
            m_llmClient->listModels();
        });
    }
}

// ----------------- chat text area -----------------------------

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
        emit chatTextUpdated();
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
    m_messageInput->setEnabled(false);
    m_messageInput->setAcceptRichText(false);
    m_messageInput->setAutoFormatting(QTextEdit::AutoAll);
    // Enable drag and drop for message input
    m_messageInput->setAcceptDrops(true);
    m_messageInput->setSizePolicy( //
        QSizePolicy::Policy::Expanding,
        QSizePolicy::Policy::MinimumExpanding);

    connect(m_messageInput, &QTextEdit::textChanged, this, [this]() { //
        QTextDocument *doc = m_messageInput->document();
        m_sendButton->setEnabled(doc->isModified() && m_llmClient->hasLLModels());
    });

    return m_messageInput;
}

// Language model selection widget
inline QWidget *ChatPanelWidget::createLLMSelector(QWidget *parent)
{
    QWidget *modelWidget = new QWidget(parent);
    modelWidget->setObjectName("modelWidget");
    modelWidget->setSizePolicy( //
        QSizePolicy::Expanding,
        QSizePolicy::Minimum);

    QHBoxLayout *modelLayout = new QHBoxLayout(modelWidget);
    modelLayout->setContentsMargins(12, 12, 12, 12);

    QLabel *modelLabel = new QLabel(tr("Select Model:"), modelWidget);
    modelLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    QComboBox *comboBox = new QComboBox(modelWidget);

    comboBox->setEditable(false);
    comboBox->setEnabled(false);
    comboBox->setSizePolicy( //
        QSizePolicy::Expanding,
        QSizePolicy::Fixed);

    connect(comboBox, &QComboBox::currentIndexChanged, this, [this, comboBox](int index) {
        if (index < 0 || index >= comboBox->count())
            return;
        QVariant data = comboBox->itemData(index);
        if (data.canConvert<ModelListModel::ModelEntry>()) {
            m_llmClient->setActiveModel(data.value<ModelListModel::ModelEntry>());
        }
    });

    // Connect LLM list model - fill model selection
    connect(m_llmClient->modelList(), &ModelListModel::modelsLoaded, this, [this, comboBox]() {
        QTimer::singleShot(10, this, [this, comboBox]() {
            if (ModelListModel *list = m_llmClient->modelList()) {
                comboBox->clear();
                // Fill Model selection QComboBox
                for (int i = 0; i < list->rowCount(); i++) {
                    QModelIndex index = list->index(i, 0);
                    QVariant v = list->data(index, ModelListModel::ModelEntryRole);
                    if (!v.isNull() && v.isValid()) {
                        ModelListModel::ModelEntry entry = //
                            v.value<ModelListModel::ModelEntry>();
                        comboBox->addItem(entry.id, QVariant::fromValue(entry));
                    }
                }
                // Select first model by default
                if (comboBox->count() > 0) {
                    comboBox->setCurrentIndex(0);
                    // Save selected model entry
                    QVariant data = comboBox->itemData(0);
                    if (data.canConvert<ModelListModel::ModelEntry>()) {
                        m_llmClient->setActiveModel(data.value<ModelListModel::ModelEntry>());
                    }
                }
            }
            // unlock if model available
            m_messageInput->setEnabled(comboBox->count() > 0);
            m_sendButton->setEnabled(comboBox->count() > 0);
            m_attachButton->setEnabled(comboBox->count() > 0);
            comboBox->setEnabled(comboBox->count() > 0);
        });
    });

    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(comboBox);
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
    m_attachButton->setEnabled(false);

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
            m_llmClient->cancelRequest();
            onHideProgressPopup();
            return;
        }

        QList<LLMChatClient::SendParameters> messages;
#if 0
        // Chat history
        for (int i = 0; i < m_chatModel->rowCount(); i++) {
            ChatMessage *cm = m_chatModel->messageAt(i);
            if (cm->isUser()) {
                messages.append({
                    .role = cm->role(),
                    .content = cm->content(),
                });
            } else {
                foreach (auto t, cm->toolCalls()) {
                    messages.append({
                        .role = cm->role(),
                        .toolName = t.functionName(),
                        .toolQuery = t.functionName(),
                        .toolResult = cm->toolContent(),
                    });
                }
            }
        }
#endif
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

        // Add sender bubble
        ChatMessage cm(m_chatModel);
        cm.setContent(question);
        cm.setRole(ChatMessage::Role::ChatRole);
        cm.setId(QStringLiteral("CPW-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        cm.setSystemFingerprint(cm.id());
        cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
        cm.setModel(m_llmClient->activeModel().id);
        m_chatModel->appendMessage(cm);

        // Clear input
        m_fileListModel->clear();
        m_messageInput->clear();

        // Show progress popup
        // onShowProgressPopup();

        m_sendButton->setText(tr("Stop"));
        m_messageInput->setEnabled(false);
        m_attachButton->setEnabled(false);
        m_isConversating = true;

        // Send JSON to LLM server
        m_llmClient->sendChat(messages, true);
    });

    return m_sendButton;
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

// ---------------- LLM Connection Management Events ----------------

void ChatPanelWidget::onConnectionChanged(LLMConnection *connection)
{
    m_activeConnection = connection;
    if (m_activeConnection && m_activeConnection->isValid()) {
        QTimer::singleShot(10, this, [this]() {
            m_llmClient->setConnection(m_activeConnection);
            m_llmClient->listModels();
        });
    }
}

// ---------------- Chat Message Events ----------------------------

void ChatPanelWidget::onUpdateChatText(int index, ChatMessage *message)
{
    qDebug().noquote() << "[ChatPanelWidget] onUpdateChatText index:" << index //
                       << "id:" << message->id() << "msg:" << message->content();
    // ensure UI thread
    //QTimer::singleShot(10, this, [index, message, this]() {
    m_chatView->appendMessage(message);
    //});
}

inline void ChatPanelWidget::connectChatModel()
{
    connect(m_chatModel, &ChatModel::messageAdded, this, [this](ChatMessage *message) { //
        onUpdateChatText(-1, message);
    });
    connect(m_chatModel, &ChatModel::messageChanged, this, [this](ChatMessage *message, int index) { //
        onUpdateChatText(index, message);
    });
    connect(m_chatModel, &ChatModel::messageRemoved, this, [](int) { //
        //
    });
    // Get notified about message parser events
    connect(m_chatModel, &ChatModel::streamCompleted, this, &ChatPanelWidget::onHideProgressPopup, Qt::QueuedConnection);
    connect(m_chatModel, &ChatModel::toolRequest, this, &ChatPanelWidget::onToolRequest, Qt::QueuedConnection);
}

inline void ChatPanelWidget::reportLLMError(QNetworkReply::NetworkError error, const QString &message)
{
    qCritical().noquote() << "[ChatPanelWidget]" << message << error;

    ChatMessage cm(m_chatModel);
    cm.setRole(ChatMessage::Role::SystemRole);
    cm.setId(QStringLiteral("CPW-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    cm.setSystemFingerprint(cm.id());
    cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
    cm.setModel(m_llmClient->activeModel().id);
    cm.setContent(QStringLiteral("Error: %1\n%2").arg(error).arg(message));

    m_chatModel->appendMessage(cm);
    m_llmClient->cancelRequest();

    QTimer::singleShot(10, this, [this]() { //
        onHideProgressPopup();
    });
}

inline void ChatPanelWidget::connectLLMClient()
{
    connect(m_llmClient, &LLMChatClient::networkError, this, [this](QNetworkReply::NetworkError error, const QString &message) { //
        reportLLMError(error, message);
    });
    connect(m_llmClient, &LLMChatClient::errorOccurred, this, [this](const QString &message) { //
        reportLLMError(QNetworkReply::NetworkError::OperationCanceledError, message);
    });
    // Link incomming data from LLM to chat model
    connect(m_llmClient, &LLMChatClient::parseDataStream, m_chatModel, &ChatModel::onParseDataStream);
    connect(m_llmClient, &LLMChatClient::parseDataObject, m_chatModel, &ChatModel::onParseMessageObject);
}

// ---------------- Progress Popup Methods ----------------------

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

// Triggered by LLM client / ChatTextWidget
void ChatPanelWidget::onHideProgressPopup()
{
    m_isConversating = false;

    // Disable blur effect
    if (m_progressPopup) {
        m_progressPopup->setBlurEffect(false);
        m_progressPopup->hide();
        m_progressPopup->deleteLater();
        m_progressPopup = nullptr;
    }

    if (m_llmClient->hasLLModels()) {
        m_sendButton->setText(tr("Send"));
        m_sendButton->setEnabled(true);
        m_messageInput->setEnabled(true);
        m_attachButton->setEnabled(true);
    }
}

// ---------------- Drag and Drop Events --------------------------

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

// ---------------- Chat Tooling Events ----------------------------

void ChatPanelWidget::onToolRequest(ChatMessage *message, const ToolCallEntry &toolCall)
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
        case ToolCallEntry::ToolType::Function: {
            toolResult = toolService.execute(tool, toolCall.arguments());
            break;
        }
        case ToolCallEntry::ToolType::Resuource: {
            toolResult = toolService.execute(tool, toolCall.arguments());
            break;
        }
        case ToolCallEntry::ToolType::Prompt: {
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

    ChatMessage cm(m_chatModel);
    cm.setRole(ChatMessage::Role::ToolingRole);
    cm.setId(message->id());
    cm.setSystemFingerprint(cm.id());
    cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
    cm.setModel(m_llmClient->activeModel().id);
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

    m_chatModel->appendMessage(cm);

    // Send message to LLM
    LLMChatClient::SendParameters params = {
        .role = ChatMessage::ToolingRole,
        .toolName = tool.name,
        .toolQuery = tool.title + "(" + tool.description + ")",
        .toolResult = buffer,
    };
    m_llmClient->sendChat(params, true);
}
