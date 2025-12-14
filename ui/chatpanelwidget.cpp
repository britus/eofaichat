#include <attachbutton.h>
#include <chatpanelwidget.h>
#include <chattextwidget.h>
#include <filelistmodel.h>
#include <filelistwidget.h>
#include <filenamelabel.h>
#include <mainwindow.h>
#include <progresspopup.h>
#include <toolservice.h>
#include <toolswidget.h>
#include <QApplication>
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
#include <QStandardPaths>
#include <QStyle>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>

ChatPanelWidget::ChatPanelWidget(SyntaxColorModel *scModel, ToolModel *tModel, QWidget *parent)
    : QWidget(parent)
    , m_llmclient(new LLMChatClient(tModel, new ChatModel(this), this))
    , m_syntaxModel(scModel)
    , m_toolModel(tModel)
    , m_fileListModel(new FileListModel(this))
    , m_isConversating(false)

{
    // fixed layout height
    setMinimumHeight(640);

    // create extension to language mapping once
    ChatTextTokenizer::fileExtToLanguage("cpp");
    m_syntaxModel->loadSyntaxModel();

    // Widget main area
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(12);

    // Scrollable chat area
    createChatArea(mainLayout);

    // File list widget (initially hidden)
    createFileListWidget(mainLayout);

    // Multiline input field
    createInputWidget(mainLayout);

    // Language Model Selection Widget
    createLLMSelector(mainLayout);

    // Buttons row
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    // Align layout to the right
    buttonLayout->setAlignment(Qt::AlignRight);
    // First add spacer to push buttons to the right
    buttonLayout->addStretch();
    createAttachButton(buttonLayout);
    createToolsButton(buttonLayout);
    createSendButton(buttonLayout);
    mainLayout->addLayout(buttonLayout);

    // assign layout to widget
    setLayout(mainLayout);

    // Connect chat message model events
    connectChatModel();

    // Connect LLM client events
    connectLLMClient();

    // Connect LLM server
    QTimer::singleShot(10, this, [this]() {
        m_llmclient->setApiKey("lm-studio");
        m_llmclient->setServerUrl("http://localhost:1234");
        m_llmclient->listModels();
    });
}

inline void ChatPanelWidget::createChatArea(QVBoxLayout *mainLayout)
{
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_messagesContainer = new QWidget(this);
    m_messagesContainer->setObjectName("messagesContainer");
    m_messagesLayout = new QVBoxLayout(m_messagesContainer);
    m_messagesLayout->setAlignment(Qt::AlignTop);

    m_messagesContainer->setLayout(m_messagesLayout);
    m_scrollArea->setWidget(m_messagesContainer);
    mainLayout->addWidget(m_scrollArea, 1);

    // ----------------- chat text area -----------------
    m_chatWidget = new ChatTextWidget(m_messagesContainer, m_syntaxModel);
    m_messagesLayout->addWidget(m_chatWidget);

    connect(m_chatWidget, &ChatTextWidget::documentUpdated, this, [this]() { //
        onHideProgressPopup();
    });
}

inline void ChatPanelWidget::createFileListWidget(QVBoxLayout *mainLayout)
{
    m_fileListWidget = new FileListWidget(m_fileListModel, this);
    m_fileListWidget->setObjectName("fileListWidget");
    m_fileListWidget->setModel(m_fileListModel);
    m_fileListWidget->setSizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Minimum);
    m_fileListWidget->setMaximumHeight(100);
    m_fileListWidget->setVisible(false);
    mainLayout->addWidget(m_fileListWidget);
}

inline void ChatPanelWidget::createInputWidget(QVBoxLayout *mainLayout)
{
    m_messageInput = new QTextEdit(this);
#ifdef Q_OS_MACOS
    m_messageInput->setPlaceholderText(tr("Type your message... | Option+Enter (⌥ + ⏎) submit."));
#else
    messageInput->setPlaceholderText(tr("Type your message... | Alt+Enter submit."));
#endif
    m_messageInput->setFixedHeight(100); // adjustable height
    m_messageInput->setAcceptRichText(false);
    m_messageInput->setAutoFormatting(QTextEdit::AutoAll);
    // Enable drag and drop for message input
    m_messageInput->setAcceptDrops(true);
    mainLayout->addWidget(m_messageInput);
}

// Language model selection widget
inline void ChatPanelWidget::createLLMSelector(QVBoxLayout *mainLayout)
{
    QWidget *modelWidget;
    QComboBox *comboBox;
    QLabel *modelLabel;

    modelWidget = new QWidget(this);
    modelWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    modelWidget->setObjectName("modelWidget");

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
        if (data.canConvert<ModelEntry>()) {
            m_llmclient->setActiveModel(data.value<ModelEntry>());
        }
    });

    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(comboBox);

    mainLayout->addWidget(modelWidget);

    // Connect LLM list model
    connect(m_llmclient->llmModels(), &ModelListModel::modelsLoaded, this, [this, comboBox]() {
        QTimer::singleShot(10, this, [this, comboBox]() {
            const QList<ModelEntry> &models = m_llmclient->llmModels()->modelList();
            // Fill Model selection QComboBox
            comboBox->clear();
            for (const ModelEntry &entry : models) {
                comboBox->addItem(entry.id, QVariant::fromValue(entry));
            }
            // Select first model by default
            if (comboBox->count() > 0) {
                comboBox->setCurrentIndex(0);
                // Save selected model entry
                QVariant data = comboBox->itemData(0);
                if (data.canConvert<ModelEntry>()) {
                    m_llmclient->setActiveModel(data.value<ModelEntry>());
                }
            }
        });
    });
}

inline void ChatPanelWidget::createAttachButton(QHBoxLayout *buttonLayout)
{
    m_attachButton = new AttachButton(tr("Attach"), this);
    buttonLayout->addWidget(m_attachButton);

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
}

inline void ChatPanelWidget::createToolsButton(QHBoxLayout *buttonLayout)
{
    QPushButton *toolsButton = new QPushButton(tr("Tools"), this);
    toolsButton->setEnabled(m_toolModel->rowCount() > 0);
    buttonLayout->addWidget(toolsButton);

    connect(toolsButton, &QPushButton::clicked, this, [this]() {
        // Create a new window for the tools
        if (m_toolsWindow == nullptr) {
            m_toolsWindow = new QMainWindow(this->window());
            m_toolsWindow->setWindowFlag(Qt::WindowType::Tool, true);
            m_toolsWindow->setWindowTitle(qApp->applicationDisplayName() + " - " + tr("Tools"));
            m_toolsWindow->setWindowIcon(QIcon(":/assets/eofaichat.png"));
            // Set window properties
            m_toolsWindow->setFixedSize(380, 420);
            m_toolsWindow->resize(m_toolsWindow->size());
            // Create the ToolsWidget
            ToolsWidget *toolsWidget = new ToolsWidget(m_toolModel, m_toolsWindow);
            // Set the ToolsWidget as the central widget of the window
            m_toolsWindow->setCentralWidget(toolsWidget);
        }
        // Show the window
        m_toolsWindow->setWindowState(Qt::WindowState::WindowActive);
        m_toolsWindow->show();
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
}

inline void ChatPanelWidget::createSendButton(QHBoxLayout *buttonLayout)
{
    m_sendButton = new QPushButton(tr("Send"), this);
    buttonLayout->addWidget(m_sendButton);

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
                    .message = cm->content(),
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
        QByteArray question = m_messageInput->toMarkdown().trimmed().toUtf8();
        if (question.isEmpty())
            return;

        // New question
        messages.append({
            .role = ChatMessage::Role::UserRole,
            .message = question,
        });

        // load file attachments
        if (m_fileListModel->rowCount() > 0) {
            QByteArray content;
            for (int i = 0; i < m_fileListModel->rowCount(); i++) {
                if (FileItem *_item = dynamic_cast<FileItem *>(m_fileListModel->item(i))) {
                    messages.append({
                        .role = ChatMessage::Role::AssistantRole,
                        .toolName = "attachment",
                        .toolQuery = _item->fileInfo().absoluteFilePath(),
                        .toolResult = m_fileListModel->readFileContent(i),
                    });
                }
            }
        }

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
}

// ==================================================

void ChatPanelWidget::onUpdateChatText(int index, ChatMessage *message)
{
    qDebug().noquote() << "[ChatPanelWidget] onUpdateChatText index:" << index //
                       << "id:" << message->id();

    // ensure UI thread
    QTimer::singleShot(10, this, [index, message, this]() {
        if (index < 0) {
            m_chatWidget->appendMessage(message);
        } else {
            m_chatWidget->updateMessage(message);
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
