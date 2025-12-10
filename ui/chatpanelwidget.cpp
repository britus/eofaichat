#include <attachbutton.h>
#include <chatpanelwidget.h>
#include <chattextwidget.h>
#include <filelistmodel.h>
#include <filelistwidget.h>
#include <filenamelabel.h>
#include <mainwindow.h>
#include <progresspopup.h>
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

ChatPanelWidget::ChatPanelWidget(QWidget *parent)
    : QWidget(parent)
    , llmclient(new LLMChatClient(parent))
    , llmModels(new ModelListModel(parent))
    , chatModel(new ChatModel(parent))
    , syntaxModel(new SyntaxColorModel(parent))
    , fileListModel(new FileListModel(parent))
    , toolModel(new ToolModel(parent))
{
    setStyleSheet(R"(
        QWidget {
            background-color: #2b2b2b;
            color: #eeeeee;
        }
    )");

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

inline void ChatPanelWidget::connectChatModel()
{
    connect(chatModel, &ChatModel::messageAdded, this, [this](ChatMessage *message) {
        qDebug().noquote() << "[CHATWIDGET] messageAdded message:" << message->id();
        // ensure UI thread
        QTimer::singleShot(10, this, [message, this]() {
            QString content = message->content();
            if (content.startsWith("[") && content.endsWith("]")) {
                content = "```json" + content + "```";
            }
#if 0
            ChatTextWidget *chatWidget = new ChatTextWidget(chatWidgetContainer, syntaxModel);
            connect(chatWidget, &ChatTextWidget::documentUpdated, this, [this]() { hideProgressPopup(); });
            m_messages[message] = chatWidget; // hold for modify, delete etc.
            // Create a horizontal layout to control alignment
            if (message->role() == ChatMessage::ChatRole) {
                // Right-align Chat role messages
                QHBoxLayout *widgetLayout = new QHBoxLayout();
                widgetLayout->setContentsMargins(0, 0, 0, 0);
                widgetLayout->addStretch();
                widgetLayout->addWidget(chatWidget);
                // Add the layout to the container using addLayout
                static_cast<QVBoxLayout *>(chatWidgetContainer->layout())->addLayout(widgetLayout);
            } else {
                // Full width for other roles
                static_cast<QVBoxLayout *>(chatWidgetContainer->layout())->addWidget(chatWidget);
            }
#endif
            chatWidget->setMessage(message->content(), message->role() == ChatMessage::ChatRole);
        });
    });
    connect(chatModel, &ChatModel::messageRemoved, this, [](int index) { //
        qDebug().noquote() << "[CHATWIDGET] messageRemoved row:" << index;
    });
    connect(chatModel, &ChatModel::messageChanged, this, [](int index, ChatMessage *message) { //
        qDebug().noquote() << "[CHATWIDGET] messageChanged row:" << index << message->id();
    });
}

inline void ChatPanelWidget::createChatArea(QVBoxLayout *mainLayout)
{
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(R"(
        background-color: #3a3a3a;
        border: 1px solid #444;
        border-radius: 8px;
        margin:0;
    )");

    messagesContainer = new QWidget(this);
    messagesLayout = new QVBoxLayout(messagesContainer);
    messagesLayout->setAlignment(Qt::AlignTop);

    messagesContainer->setLayout(messagesLayout);
    scrollArea->setWidget(messagesContainer);
    mainLayout->addWidget(scrollArea, 1);

    // ----------------- chat text area -----------------
#if 0
    QVBoxLayout *chatWidgetContainerLayout = new QVBoxLayout(this);
    chatWidgetContainerLayout->setContentsMargins(0, 0, 0, 0);
    chatWidgetContainerLayout->setSpacing(12);
    chatWidgetContainer = new QWidget(this);
    chatWidgetContainer->setStyleSheet(R"(
        background-color: #3a3a3a;
        border: 0px;
    )");
    chatWidgetContainer->setLayout(chatWidgetContainerLayout);
#endif

    chatWidget = new ChatTextWidget(messagesContainer, syntaxModel);
    messagesLayout->addWidget(chatWidget);

    connect(chatWidget, &ChatTextWidget::documentUpdated, this, [this]() { //
        hideProgressPopup();
    });
}

inline void ChatPanelWidget::createFileListWidget(QVBoxLayout *mainLayout)
{
    fileListWidget = new FileListWidget(fileListModel, this);
    fileListWidget->setModel(fileListModel);
    fileListWidget->setSizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::Minimum);
    fileListWidget->setMaximumHeight(180);
    fileListWidget->setVisible(false);
    mainLayout->addWidget(fileListWidget);
}

inline void ChatPanelWidget::createInputWidget(QVBoxLayout *mainLayout)
{
    messageInput = new QTextEdit(this);
    messageInput->setPlaceholderText("Type your message...");
    messageInput->setFixedHeight(100); // adjustable height
    messageInput->setAcceptRichText(false);
    messageInput->setAutoFormatting(QTextEdit::AutoAll);
    messageInput->setStyleSheet(R"(
        QTextEdit {
            font-family: monospace;
            font-size: 16pt;
            background-color: #3a3a3a;
            border: 1px solid #444;
            border-radius: 8px;
        }
    )");
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
    modelWidget->setObjectName("msw");
    modelWidget->setStyleSheet(R"(
        QWidget#msw {
            background-color: #3a3a3a;
            color: #dddddd;
            border: 1px solid #555;
            border-radius: 8px;
            padding: 8px;
        }
        QLabel {
            background-color: #3a3a3a;
            color: #dddddd;
        }
    )");
    modelWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QHBoxLayout *modelLayout = new QHBoxLayout(modelWidget);
    modelLayout->setContentsMargins(12, 12, 12, 12);

    modelLabel = new QLabel("Select Model:", modelWidget);
    modelLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    comboBox = new QComboBox(modelWidget);
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(comboBox, &QComboBox::currentIndexChanged, this, [this, comboBox](int index) {
        if (index < 0 || index >= comboBox->count())
            return;
        QVariant data = comboBox->itemData(index);
        if (data.canConvert<ModelEntry>()) {
            selectedModelEntry = data.value<ModelEntry>();
            qDebug() << "Selected model:" << selectedModelEntry.id;
        }
    });

    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(comboBox);

    mainLayout->addWidget(modelWidget);

    // Connect LLM list model
    connect(llmModels, &ModelListModel::modelsLoaded, this, [this, comboBox]() {
        const QList<ModelEntry> &models = llmModels->modelList();

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
                selectedModelEntry = data.value<ModelEntry>();
            }
        }
    });
}

inline void ChatPanelWidget::createAttachButton(QHBoxLayout *buttonLayout)
{
    AttachButton *attachButton = new AttachButton("Attach", this);
    attachButton->setMinimumHeight(24);
    attachButton->setMinimumWidth(90);
    attachButton->setStyleSheet(R"(
        QPushButton {
            background-color: #3a3a3a;
            border: 1px solid #555;
            border-radius: 8px;
            padding: 8px;
            font-size: 16px;
        }
        QPushButton:hover {
            background-color: #444;
        }
    )");
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
    QPushButton *toolsButton = new QPushButton("Tools", this);
    toolsButton->setMinimumHeight(24);
    toolsButton->setMinimumWidth(90);
    toolsButton->setStyleSheet(R"(
        QPushButton {
            background-color: #3a3a3a;
            border: 1px solid #555;
            border-radius: 8px;
            padding: 8px;
            font-size: 16px;
        }
        QPushButton:hover {
            background-color: #444;
        }
    )");
    buttonLayout->addWidget(toolsButton);

    connect(toolsButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "Tools action triggered";

        // Create a new window for the tools
        if (toolsWindow == nullptr) {
            toolsWindow = new QMainWindow(this->window());
            toolsWindow->setWindowFlag(Qt::WindowType::Tool, true);
            toolsWindow->setWindowTitle("Tools");
            // Set window properties
            toolsWindow->setFixedSize(450, 320);
            toolsWindow->resize(toolsWindow->size());
            // Create the ToolsWidget
            ToolsWidget *toolsWidget = new ToolsWidget(toolsWindow);
            // Set up the tool model (this would typically be connected to a real model)
            toolsWidget->setToolModel(toolModel);
            // Set the ToolsWidget as the central widget of the window
            toolsWindow->setCentralWidget(toolsWidget);
        }

        // Show the window
        toolsWindow->setWindowState(Qt::WindowState::WindowActive);
        toolsWindow->show();
    });

    // Tools, Resource, Prompts configuration
    connect(toolModel, &ToolModel::toolAdded, this, [this, toolsButton](const ToolEntry &entry) { //
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
    QPushButton *sendButton = new QPushButton("Send", this);
    sendButton->setMinimumHeight(24);
    sendButton->setMinimumWidth(90);
    sendButton->setStyleSheet(R"(
        QPushButton {
            background-color: #3a3a3a;
            border: 1px solid #555;
            border-radius: 8px;
            padding: 8px;
            font-size: 16px;
        }
        QPushButton:hover {
            background-color: #444;
        }
    )");
    buttonLayout->addWidget(sendButton);

    connect(sendButton, &QPushButton::clicked, this, [this]() {
        QString llmodel = selectedModelEntry.id;
        QString text = messageInput->toPlainText().trimmed();
        if (text.isEmpty())
            return;

        if (fileListModel->rowCount() > 0) {
            QByteArray content;
            fileListModel->loadContentOfFiles(content);
            if (!content.isEmpty()) {
                text = text.append("\r\n");
                text = text.append(content);
            }
        }

        // Add sender bubble using ChatTextWidget
        ChatMessage cm;
        cm.setContent(text);
        cm.setRole(ChatMessage::Role::ChatRole);
        cm.setId(QStringLiteral("CS-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        cm.setSystemFingerprint(cm.id());
        cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
        cm.setModel(llmodel);
        chatModel->addMessage(cm);

        // Clear input
        messageInput->clear();
        fileListModel->clear();

        // Show progress popup
        showProgressPopup();

        // Emit or send JSON for network layer
        llmclient->sendChat(llmodel, text);
    });
}

inline void ChatPanelWidget::connectLLMClient()
{
    connect(llmclient, &LLMChatClient::networkError, this, [this](QNetworkReply::NetworkError error, const QString &message) {
        qDebug().noquote() << "[CHATWIDGET]" << message << error;
        ChatMessage cm;
        cm.setRole(ChatMessage::Role::AssistantRole);
        cm.setId(QStringLiteral("CS-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        cm.setSystemFingerprint(cm.id());
        cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
        cm.setModel(selectedModelEntry.id);
        cm.setContent(QStringLiteral("Error: %1\n%2").arg(error).arg(message));
        chatModel->addMessage(cm);
    });
    connect(llmclient, &LLMChatClient::errorOccurred, this, [this](const QString &error) {
        qDebug().noquote() << "[CHATWIDGET]" << error;
        ChatMessage cm;
        cm.setRole(ChatMessage::Role::AssistantRole);
        cm.setId(QStringLiteral("CS-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        cm.setSystemFingerprint(cm.id());
        cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
        cm.setModel(selectedModelEntry.id);
        cm.setContent(error);
        chatModel->addMessage(cm);
    });
    connect(llmclient, &LLMChatClient::chatCompletionReceived, this, [this](const QJsonObject &response) { //
        chatModel->addMessageFromJson(response);
    });
    connect(llmclient, &LLMChatClient::modelListReceived, this, [this](const QJsonArray &models) { //
        llmModels->loadFrom(models);
    });
    connect(llmclient, &LLMChatClient::streamingDataReceived, this, [this](const QString &data) {
        ChatMessage cm;
        cm.setRole(ChatMessage::Role::AssistantRole);
        cm.setId(QStringLiteral("CS-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
        cm.setSystemFingerprint(cm.id());
        cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
        cm.setModel(selectedModelEntry.id);
        cm.setContent(data);
        chatModel->addMessage(cm);
    });
}

// ---------------- Progress Popup Methods ----------------
void ChatPanelWidget::showProgressPopup()
{
    if (progressPopup)
        delete progressPopup;
    // Initialize progress popup
    progressPopup = new ProgressPopup(this);
    // Enable blur effect on central widget
    progressPopup->setBlurEffect(true);
    // Show centered popup
    progressPopup->showCentered();
}

void ChatPanelWidget::hideProgressPopup()
{
    // Disable blur effect
    if (progressPopup) {
        progressPopup->setBlurEffect(false);
        progressPopup->hide();
        delete progressPopup;
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
