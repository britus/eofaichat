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
    , llmclient(new LLMChatClient(tModel, new ChatModel(this), this))
    , syntaxModel(scModel)
    , toolModel(tModel)
    , fileListModel(new FileListModel(this))

{
    // fixed layout height
    setMinimumHeight(640);

    // create extension to language mapping once
    ChatTextTokenizer::fileExtToLanguage("cpp");
    syntaxModel->loadSyntaxModel();

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
        llmclient->setApiKey("lm-studio");
        llmclient->setServerUrl("http://localhost:1234");
        llmclient->listModels();
    });
}

// ==================================================

void ChatPanelWidget::onUpdateChatText(int index, ChatMessage *message, bool removed)
{
    qDebug().noquote() << "[ChatPanelWidget] onUpdateChatText index:" << index //
                       << "id:" << message->id() << "removed:" << removed;

    // ensure UI thread
    QTimer::singleShot(10, this, [index, message, removed, this]() {
        if (removed) {
            chatWidget->removeMessage(message);
        } else if (index < 0) {
            chatWidget->appendMessage(message);
        } else {
            chatWidget->updateMessage(message);
        }
    });
}

inline void ChatPanelWidget::connectChatModel()
{
    connect(llmclient->chatModel(), &ChatModel::messageAdded, this, [this](ChatMessage *message) { //
        onUpdateChatText(-1, message);
    });
    connect(llmclient->chatModel(), &ChatModel::messageChanged, this, [this](int index, ChatMessage *message) { //
        onUpdateChatText(index, message);
    });
    connect(llmclient->chatModel(), &ChatModel::messageRemoved, this, [this](int index) { //
        onUpdateChatText(index, llmclient->chatModel()->messageAt(index), true);
    });
}

inline void ChatPanelWidget::createChatArea(QVBoxLayout *mainLayout)
{
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    messagesContainer = new QWidget(this);
    messagesContainer->setObjectName("messagesContainer");
    messagesLayout = new QVBoxLayout(messagesContainer);
    messagesLayout->setAlignment(Qt::AlignTop);

    messagesContainer->setLayout(messagesLayout);
    scrollArea->setWidget(messagesContainer);
    mainLayout->addWidget(scrollArea, 1);

    // ----------------- chat text area -----------------
    chatWidget = new ChatTextWidget(messagesContainer, syntaxModel);
    messagesLayout->addWidget(chatWidget);

    connect(chatWidget, &ChatTextWidget::documentUpdated, this, [this]() { //
        onHideProgressPopup();
    });
}

inline void ChatPanelWidget::createFileListWidget(QVBoxLayout *mainLayout)
{
    fileListWidget = new FileListWidget(fileListModel, this);
    fileListWidget->setObjectName("fileListWidget");
    fileListWidget->setModel(fileListModel);
    fileListWidget->setSizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Minimum);
    fileListWidget->setMaximumHeight(100);
    fileListWidget->setVisible(false);
    mainLayout->addWidget(fileListWidget);
}

inline void ChatPanelWidget::createInputWidget(QVBoxLayout *mainLayout)
{
    messageInput = new QTextEdit(this);
#ifdef Q_OS_MACOS
    messageInput->setPlaceholderText(tr("Type your message... | Option+Enter (⌥ + ⏎) submit."));
#else
    messageInput->setPlaceholderText(tr("Type your message... | Alt+Enter submit."));
#endif
    messageInput->setFixedHeight(100); // adjustable height
    messageInput->setAcceptRichText(false);
    messageInput->setAutoFormatting(QTextEdit::AutoAll);
    // Enable drag and drop for message input
    messageInput->setAcceptDrops(true);
    mainLayout->addWidget(messageInput);
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
            llmclient->setActiveModel(data.value<ModelEntry>());
        }
    });

    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(comboBox);

    mainLayout->addWidget(modelWidget);

    // Connect LLM list model
    connect(llmclient->llmModels(), &ModelListModel::modelsLoaded, this, [this, comboBox]() {
        QTimer::singleShot(10, this, [this, comboBox]() {
            const QList<ModelEntry> &models = llmclient->llmModels()->modelList();
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
                    llmclient->setActiveModel(data.value<ModelEntry>());
                }
            }
            onHideProgressPopup();
        });
    });
}

inline void ChatPanelWidget::createAttachButton(QHBoxLayout *buttonLayout)
{
    AttachButton *attachButton = new AttachButton(tr("Attach"), this);
    buttonLayout->addWidget(attachButton);

    connect(attachButton, &AttachButton::clicked, this, [this]() {
        QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        if (lastDirectory.isEmpty())
            lastDirectory = homePath;
        QString file = QFileDialog::getOpenFileName( //
            this,
            "Select File to Attach",
            lastDirectory,
            "Source Files (*.cpp *.c *.h *.hpp *.cc *.cxx *.hxx *.cs *.java *.py *.js *.ts *.php *.rb *.pl *.sh *.go *.rs *.swift *.kt *.scala *.r *.m *.mm);;"
            "Text Files (*.txt *.md *.log);;"
            "Configuration Files (*.json *.xml *.yaml *.yml *.ini *.cfg);;"
            "All Files (*)");
        if (file.isEmpty())
            return;
        // remember path
        QFileInfo fi(file);
        lastDirectory = fi.filePath();
        // Add file to the file list model
        fileListModel->addFile(file, style()->standardIcon(QStyle::SP_FileIcon));
    });
    connect(attachButton, &AttachButton::fileDropped, this, [this](const QList<QUrl> &urls) {
        const QIcon icon = style()->standardIcon(QStyle::SP_FileIcon);
        foreach (const QUrl &url, urls) {
            QString filePath = url.toLocalFile();
            if (!filePath.isEmpty()) {
                // Add to file list model
                fileListModel->addFile(filePath, icon);
            }
        }
    });
}

inline void ChatPanelWidget::createToolsButton(QHBoxLayout *buttonLayout)
{
    QPushButton *toolsButton = new QPushButton(tr("Tools"), this);
    toolsButton->setEnabled(toolModel->rowCount() > 0);
    buttonLayout->addWidget(toolsButton);

    connect(toolsButton, &QPushButton::clicked, this, [this]() {
        // Create a new window for the tools
        if (toolsWindow == nullptr) {
            toolsWindow = new QMainWindow(this->window());
            toolsWindow->setWindowFlag(Qt::WindowType::Tool, true);
            toolsWindow->setWindowTitle(qApp->applicationDisplayName() + " - Tools");
            toolsWindow->setWindowIcon(QIcon(":/assets/eofaichat.png"));
            // Set window properties
            toolsWindow->setFixedSize(380, 420);
            toolsWindow->resize(toolsWindow->size());
            // Create the ToolsWidget
            ToolsWidget *toolsWidget = new ToolsWidget(toolModel, toolsWindow);
            // Set the ToolsWidget as the central widget of the window
            toolsWindow->setCentralWidget(toolsWidget);
        }
        // Show the window
        toolsWindow->setWindowState(Qt::WindowState::WindowActive);
        toolsWindow->show();
    });

    // Tools, Resource, Prompts configuration
    connect(toolModel, &ToolModel::toolAdded, this, [this, toolsButton](const ToolModel::ToolModelEntry &entry) { //
        qDebug("Tool config type %d added: %s", entry.type, qPrintable(entry.name));
        toolsButton->setEnabled(toolModel->hasExecutables()  //
                                || toolModel->hasResources() //
                                || toolModel->hasPrompts());
    });
    connect(toolModel, &ToolModel::toolRemoved, this, [this, toolsButton](int) { //
        toolsButton->setEnabled(toolModel->hasExecutables()                      //
                                || toolModel->hasResources()                     //
                                || toolModel->hasPrompts());
    });

    toolModel->loadToolsConfig();
}

inline void ChatPanelWidget::createSendButton(QHBoxLayout *buttonLayout)
{
    sendButton = new QPushButton(tr("Send"), this);
    buttonLayout->addWidget(sendButton);

    connect(sendButton, &QPushButton::clicked, this, [this]() {
        QByteArray question = messageInput->toPlainText().trimmed().toUtf8();
        if (question.isEmpty())
            return;

        // app new conversation
        QByteArray text;
        text.append(question);

        // load previous chat messages
        QByteArray previusChat = llmclient->chatModel()->chatContent();
        if (previusChat.length() > 0) {
            text.append("#Previous conversation:\n");
            text.append(previusChat);
        }

        // load file attachments
        if (fileListModel->rowCount() > 0) {
            QByteArray content;
            text.append("#Attached files:\n");
            fileListModel->loadContentOfFiles(content);
            if (!content.isEmpty()) {
                text = text.append("\n");
                text = text.append(content);
            }
        }

        // Add sender bubble
        ChatMessage cm(llmclient->chatModel());
        cm.setContent(question);
        cm.setRole(ChatMessage::Role::ChatRole);
        cm.setId(QStringLiteral("CPW-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        cm.setSystemFingerprint(cm.id());
        cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
        cm.setModel(llmclient->activeModel().id);
        llmclient->chatModel()->addMessage(cm);

        // Clear input
        messageInput->clear();
        fileListModel->clear();

        // Show progress popup
        onShowProgressPopup();

        // Send JSON for network layer
        llmclient->sendChat(cm.model(), text, true);
    });
}

inline void ChatPanelWidget::connectLLMClient()
{
    connect(llmclient, &LLMChatClient::toolRequest, this, &ChatPanelWidget::onToolRequest, Qt::QueuedConnection);
    connect(llmclient, &LLMChatClient::streamCompleted, this, &ChatPanelWidget::onHideProgressPopup, Qt::QueuedConnection);
    connect(llmclient, &LLMChatClient::networkError, this, [this](QNetworkReply::NetworkError error, const QString &message) {
        qCritical().noquote() << "[ChatPanelWidget]" << message << error;
        ChatMessage cm(llmclient->chatModel());
        cm.setRole(ChatMessage::Role::AssistantRole);
        cm.setId(QStringLiteral("CPW-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        cm.setSystemFingerprint(cm.id());
        cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
        cm.setModel(llmclient->activeModel().id);
        cm.setContent(QStringLiteral("Error: %1\n%2").arg(error).arg(message));
        llmclient->chatModel()->addMessage(cm);
        onHideProgressPopup();
    });
    connect(llmclient, &LLMChatClient::errorOccurred, this, [this](const QString &error) {
        qCritical().noquote() << "[ChatPanelWidget]" << error;
        ChatMessage cm(llmclient->chatModel());
        cm.setRole(ChatMessage::Role::AssistantRole);
        cm.setId(QStringLiteral("CPW-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        cm.setSystemFingerprint(cm.id());
        cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
        cm.setModel(llmclient->activeModel().id);
        cm.setContent(error);
        llmclient->chatModel()->addMessage(cm);
        onHideProgressPopup();
    });
}

// ---------------- Progress Popup Methods ----------------
void ChatPanelWidget::onShowProgressPopup()
{
    if (progressPopup)
        progressPopup->deleteLater();
    // Initialize progress popup
    progressPopup = new ProgressPopup(this);
    // Enable blur effect on central widget
    progressPopup->setBlurEffect(true);
    // Show centered popup
    progressPopup->showCentered();
}

void ChatPanelWidget::onHideProgressPopup()
{
    // Disable blur effect
    if (progressPopup) {
        progressPopup->setBlurEffect(false);
        progressPopup->hide();
        progressPopup->deleteLater();
        progressPopup = nullptr;
    }
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
                QPoint messageInputPos = messageInput->mapTo(this, QPoint(0, 0));
                QRect messageInputRect(messageInputPos, messageInput->size());
                if (messageInputRect.contains(pos)) {
                    // Drop on message input - insert full path
                    QString text = messageInput->toPlainText();
                    messageInput->setText(text + filePath);
                } else {
                    // Drop on main widget - add to file list
                    fileListModel->addFile(filePath, icon);
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
            QPoint messageInputPos = messageInput->mapTo(this, QPoint(0, 0));
            QRect messageInputRect(messageInputPos, messageInput->size());
            if (messageInputRect.contains(pos)) {
                // Drop on message input - insert full path
                QString currentText = messageInput->toPlainText();
                messageInput->setText(currentText + text);
            } else {
                // Drop on main widget - add to file list
                fileListModel->addFile(text, icon);
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
        if (sendButton) {
            sendButton->click();
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
    QJsonObject result;
    QByteArray buffer;

    QString llmId = llmclient->activeModel().id;
    if (llmId.isEmpty()) {
        qCritical().noquote() << "[LLMChatClient] No LLM is activated.";
        return;
    }

    qDebug().noquote() << "[LLMChatClient] onToolRequest type:"  //
                       << toolCall.toolType()                    //
                       << "id:" << toolCall.toolCallId()         //
                       << "function:" << toolCall.functionName() //
                       << "args:" << toolCall.arguments();

    if (toolCall.functionName().isEmpty()) {
        result = toolService.createErrorResponse( //
            QStringLiteral("The function name is required."));
        goto finish;
    }

    tool = toolModel->toolByName(toolCall.functionName());
    if (tool.name.isEmpty()) {
        result = toolService.createErrorResponse( //
            QStringLiteral("Unable to find function: %1").arg(toolCall.functionName()));
        goto finish;
    }

    switch (toolCall.toolType()) {
        case ChatMessage::ToolType::Function: {
            result = toolService.execute(tool, toolCall.arguments());
            break;
        }
        case ChatMessage::ToolType::Resuource: {
            result = toolService.execute(tool, toolCall.arguments());
            break;
        }
        case ChatMessage::ToolType::Prompt: {
            result = toolService.execute(tool, toolCall.arguments());
            break;
        }
        default: {
            result = toolService.createErrorResponse( //
                QStringLiteral("Invalid tool type in function: %1").arg(toolCall.functionName()));
            goto finish;
            break;
        }
    }

    if (result.isEmpty()) {
        result = toolService.createErrorResponse( //
            QStringLiteral("Tool '%1' does not produce any results.").arg(toolCall.functionName()));
    }

    if (result.contains("structuredContent")) {
        result = result["structuredContent"].toObject();
    } else if (result.contains("content")) {
        QJsonValue contentValue = result["content"];
        if (contentValue.isArray()) {
            QJsonArray items = contentValue.toArray();
            if (!items.isEmpty() && items[0].isObject()) {
                QJsonObject itemObject = items[0].toObject();
                result = itemObject;
                // --
                //if (itemObject.contains("text")) {
                //    buffer = itemObject["text"].toString().toUtf8();
                //}
            }
        }
    }

finish:
    if (buffer.isEmpty()) {
        buffer = QJsonDocument(result).toJson(QJsonDocument::Indented);
    }

#if 0
    QJsonObject response;
    response["model"] = llmclient->activeModel().id;
    response["prompt"] = "```json\n" + QString(buffer) + "\n```\n";
    response["max_length"] = response["prompt"].toString().length();
    response["temperature"] = QJsonValue(0.7f);
    response["top_p"] = QJsonValue(0.9f);
    response["n"] = QJsonValue(1);
    response["stop"] = QJsonValue::Null; //QJsonArray() << "tools_call" << "text" << "content" << "}";

    llmclient->sendChat(llmId, QJsonDocument(response).toJson(QJsonDocument::Compact), true);
#else
    llmclient->sendChat(llmId, buffer, true);
#endif

    ChatMessage cm(llmclient->chatModel());
    cm.setRole(ChatMessage::Role::SystemRole);
    cm.setId(QStringLiteral("CPW-%1").arg(message->id()));
    cm.setSystemFingerprint(cm.id());
    cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
    cm.setModel(llmclient->activeModel().id);
    cm.setContent(tr("%1\nTool(%2:%3:%4) call completed.\n") //
                      .arg(buffer)
                      .arg(toolCall.toolType())
                      .arg(toolCall.toolCallId(), //
                           toolCall.functionName()));
    llmclient->chatModel()->addMessage(cm);
}
