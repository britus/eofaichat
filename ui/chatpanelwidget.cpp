#include "attachbutton.h"
#include <chatpanelwidget.h>
#include <chattextwidget.h>
#include <filelistmodel.h>
#include <filenamelabel.h>
#include <mainwindow.h>
#include <progresspopup.h>
#include <QApplication>
#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QGridLayout>
#include <QLabel>
#include <QStyle>

ChatPanelWidget::ChatPanelWidget(QWidget *parent)
    : QWidget(parent)
    , llmclient(new LLMChatClient(parent))
    , llmModels(new ModelListModel(parent))
    , chatModel(new ChatModel(parent))
    , syntaxModel(new SyntaxColorModel(parent))
    , fileListModel(new FileListModel(this))
{
    // Try load color model from relative file; adjust path as needed.
    QString colorFile = ":/syntaxcolors.json"; // if using resources
    if (!QFile::exists(colorFile)) {
        // fallback to local file in executable dir
        colorFile = QCoreApplication::applicationDirPath() + "/syntaxcolors.json";
    }
    if (!syntaxModel->loadFromFile(colorFile)) {
        qWarning() << "Failed to load syntax color model from" << colorFile;
    }

    // create extension to language mapping once
    ChatTextTokenizer::fileExtToLanguage("cpp");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(12);

    // ----------------- Scrollable chat area -----------------
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
    connect(chatWidget, &ChatTextWidget::documentUpdated, this, [this]() { hideProgressPopup(); });
    messagesLayout->addWidget(chatWidget);

    // ----------------- File list widget (initially hidden) -----------------
    fileListWidget = new QWidget(this);
    fileListWidget->setStyleSheet(R"(
        QWidget {
            background-color: #3a3a3a;
            border: 1px solid #444;
            border-radius: 8px;
            padding: 8px;
        }
    )");
    fileListLayout = new QGridLayout(fileListWidget);
    fileListLayout->setContentsMargins(0, 0, 0, 0);
    fileListLayout->setSpacing(4);
    mainLayout->addWidget(fileListWidget);

    // Initially hide the file list widget
    fileListWidget->setVisible(false);

    // ----------------- Multiline input field -----------------
    messageInput = new QTextEdit(this);
    messageInput->setPlaceholderText("Type your message...");
    messageInput->setFixedHeight(100); // adjustable height
    messageInput->setAcceptRichText(false);
    messageInput->setAutoFormatting(QTextEdit::AutoAll);
    messageInput->setStyleSheet(R"(
        QTextEdit {
            background-color: #3a3a3a;
            border: 1px solid #444;
            border-radius: 8px;
        }
    )");
    // Enable drag and drop for message input
    messageInput->setAcceptDrops(true);
    mainLayout->addWidget(messageInput);

    // ----------------- Language Model Selection Widget -----------------
    modelSelectionWidget = new QWidget(this);
    modelSelectionWidget->setObjectName("msw");
    modelSelectionWidget->setStyleSheet(R"(
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
    modelSelectionWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QHBoxLayout *modelLayout = new QHBoxLayout(modelSelectionWidget);
    modelLayout->setContentsMargins(12, 12, 12, 12);

    modelLabel = new QLabel("Select Model:", modelSelectionWidget);
    modelLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    modelComboBox = new QComboBox(modelSelectionWidget);
    modelComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(modelComboBox, &QComboBox::currentIndexChanged, this, &ChatPanelWidget::onModelSelected);

    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(modelComboBox);

    mainLayout->addWidget(modelSelectionWidget);

    // ----------------- Buttons row -----------------
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    // Align layout to the right
    buttonLayout->setAlignment(Qt::AlignRight);
    // First add spacer to push buttons to the right
    buttonLayout->addStretch();

    attachButton = new AttachButton("Attach", this);
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
    connect(attachButton, &AttachButton::fileDropped, this, &ChatPanelWidget::onAttachButtonFileDropped);
    buttonLayout->addWidget(attachButton);

    toolsButton = new QPushButton("Tools", this);
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
    QMenu *menu = new QMenu(this);
    QAction *toolAction = menu->addAction("Tools");
    connect(toolAction, &QAction::triggered, this, &ChatPanelWidget::onToolsClicked);
    QAction *resAction = menu->addAction("Resources");
    connect(resAction, &QAction::triggered, this, &ChatPanelWidget::onResourcesClicked);
    toolsButton->setMenu(menu);
    buttonLayout->addWidget(toolsButton);

    sendButton = new QPushButton("Send", this);
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
    connect(sendButton, &QPushButton::clicked, this, &ChatPanelWidget::onSendClicked);
    buttonLayout->addWidget(sendButton);

    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
    setStyleSheet(R"(
        QWidget {
            background-color: #2b2b2b;
            color: #dddddd;
        }
    )");

    // ==================================================
    // Connect model list
    connect(llmModels, &ModelListModel::modelsLoaded, this, [this]() {
        const QList<ModelEntry> &models = llmModels->modelList();

        // Fill Model selection QComboBox
        modelComboBox->clear();
        for (const ModelEntry &entry : models) {
            modelComboBox->addItem(entry.id, QVariant::fromValue(entry));
        }

        // Select first model by default
        if (modelComboBox->count() > 0) {
            modelComboBox->setCurrentIndex(0);
            // Save selected model entry
            QVariant data = modelComboBox->itemData(0);
            if (data.canConvert<ModelEntry>()) {
                selectedModelEntry = data.value<ModelEntry>();
            }
        }
    });

    // ==================================================
    // Connect chat message model events
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
    connect(chatModel, &ChatModel::messageRemoved, this, [](int index) { qDebug().noquote() << "[CHATWIDGET] messageRemoved row:" << index; });
    connect(chatModel, &ChatModel::messageChanged, this, [](int index, ChatMessage *message) { qDebug().noquote() << "[CHATWIDGET] messageChanged row:" << index << message->id(); });

    // ==================================================
    // Connect llmclient events
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
    connect(llmclient, &LLMChatClient::chatCompletionReceived, this, [this](const QJsonObject &response) { chatModel->addMessageFromJson(response); });
    connect(llmclient, &LLMChatClient::modelListReceived, this, [this](const QJsonArray &models) { llmModels->loadFrom(models); });
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

    // Connect file list model signals
    connect(fileListModel, &FileListModel::added, this, &ChatPanelWidget::onFileAdded);
    connect(fileListModel, &FileListModel::removed, this, &ChatPanelWidget::onFileRemoved);

    QTimer::singleShot(10, this, [this]() {
        llmclient->setApiKey("lm-studio");
        llmclient->setServerUrl("http://localhost:1234");
        llmclient->listModels();
    });
}

// ---------------- Send Button ----------------
void ChatPanelWidget::onSendClicked()
{
    QString text = messageInput->toPlainText().trimmed();
    if (text.isEmpty())
        return;

    QString llmodel = selectedModelEntry.id;

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

    // Show progress popup
    showProgressPopup();

    // Emit or send JSON for network layer
    llmclient->sendChat(llmodel, text);
}

// ---------------- Attach File Button ----------------
void ChatPanelWidget::onAttachClicked()
{
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if (lastDirectory.isEmpty())
        lastDirectory = homePath;

    QString file = QFileDialog::getOpenFileName(this,
                                                "Select File to Attach",
                                                lastDirectory,
                                                "Source Files (*.cpp *.c *.h *.hpp *.cc *.cxx *.hxx *.cs *.java *.py *.js *.ts *.php *.rb *.pl *.sh *.go *.rs *.swift *.kt *.scala *.r *.m *.mm);;"
                                                "Text Files (*.txt *.md *.log);;"
                                                "Configuration Files (*.json *.xml *.yaml *.yml *.ini *.cfg);;"
                                                "All Files (*)");
    if (file.isEmpty())
        return;

    // Add file to the file list model
    fileListModel->addFile(file);

    QByteArray msg;

    QFileInfo fi(file);
    lastDirectory = fi.filePath();

    msg.append("```");
    QString lang = ChatTextTokenizer::fileExtToLanguage(fi.suffix());
    if (!lang.isEmpty()) {
        msg.append(lang.toUtf8());
    }
    msg.append('\n');

    QFile f(file);
    if (f.open(QFile::ReadOnly)) {
        msg.append(f.readAll());
    }

    msg.append("```");
    msg.append('\n');

    // Add sender bubble using ChatTextWidget
    ChatMessage cm;
    cm.setContent(msg);
    cm.setRole(ChatMessage::Role::AssistantRole);
    cm.setId(QStringLiteral("CS-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)));
    cm.setSystemFingerprint(cm.id());
    cm.setCreated(QDateTime::currentDateTime().time().msecsSinceStartOfDay());
    cm.setModel(selectedModelEntry.id);
    chatModel->addMessage(cm);
}

// ---------------- Tools Menu Action ----------------
void ChatPanelWidget::onToolsClicked()
{
    qDebug() << "Tools action triggered";

    QMessageBox::information(this, "Tools", "Tools action clicked");
}

// ---------------- Resources Menu Action ----------------
void ChatPanelWidget::onResourcesClicked()
{
    qDebug() << "Resources action triggered";

    QMessageBox::information(this, "Resources", "Resources action clicked");
}

// ---------------- Model Selection Handler ----------------
void ChatPanelWidget::onModelSelected(int index)
{
    if (index < 0 || index >= modelComboBox->count())
        return;
    QVariant data = modelComboBox->itemData(index);
    if (data.canConvert<ModelEntry>()) {
        selectedModelEntry = data.value<ModelEntry>();
        qDebug() << "Selected model:" << selectedModelEntry.id;
    }
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
                    fileListModel->addFile(filePath);
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
                fileListModel->addFile(text);
            }
        }
        event->acceptProposedAction();
    }
}

// ---------------- Attach Button File Dropped Event ----------------
void ChatPanelWidget::onAttachButtonFileDropped(const QList<QUrl> &urls)
{
    foreach (const QUrl &url, urls) {
        QString filePath = url.toLocalFile();
        if (!filePath.isEmpty()) {
            // Add to file list model
            fileListModel->addFile(filePath);
        }
    }
}

// ---------------- File List Model Events ----------------
void ChatPanelWidget::onFileAdded(const QString &filePath)
{
    // Create a new FileNameLabel for the file
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();

    FileNameLabel *label = new FileNameLabel(fileName, fileListWidget);
    connect(label, &FileNameLabel::removeRequested, this, &ChatPanelWidget::onFileNameLabelRemoveClicked);

    // Add to layout - using a simple approach that will naturally wrap
    // The grid layout will automatically handle wrapping when items exceed the row width
    fileListLayout->addWidget(label);

    // Update visibility
    updateFileListWidgetVisibility();
}

void ChatPanelWidget::onFileRemoved(int index)
{
    // Find the widget at the index and remove it
    QWidget *widget = fileListLayout->itemAt(index)->widget();
    if (widget) {
        fileListLayout->removeWidget(widget);
        widget->deleteLater();
    }

    // Update visibility
    updateFileListWidgetVisibility();
}

void ChatPanelWidget::onFileNameLabelRemoveClicked(int index)
{
    fileListModel->removeFile(index);
}

void ChatPanelWidget::updateFileListWidgetVisibility()
{
    // Show the widget if there are files, otherwise hide it
    fileListWidget->setVisible(fileListModel->rowCount() > 0);
}