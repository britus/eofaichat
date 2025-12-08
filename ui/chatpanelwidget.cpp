#include <chatpanelwidget.h>
#include <chattextwidget.h>
#include <mainwindow.h>
#include <progresspopup.h>
#include <QApplication>
#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTextEdit>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>

//TODO: create a new separated widget class for progress popup

ChatPanelWidget::ChatPanelWidget(QWidget *parent)
    : QWidget(parent)
    , llmclient(new LLMChatClient(parent))
    , llmModels(new ModelListModel(parent))
    , chatModel(new ChatModel(parent))
    , syntaxModel(new SyntaxColorModel(parent))
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
    chatWidget = new ChatTextWidget(messagesContainer, syntaxModel);
    chatWidget->setStyleSheet(R"(
        font-family: monospace;
        background-color: transparent;
        border: 0px;
        margin:0;
    )");
    messagesLayout->addWidget(chatWidget);
    connect(chatWidget, &ChatTextWidget::documentUpdated, this, [this]() { hideProgressPopup(); });

    // ----------------- Multiline input field -----------------
    messageInput = new QTextEdit(this);
    messageInput->setPlaceholderText("Type your message...");
    messageInput->setFixedHeight(100); // adjustable height
    messageInput->setStyleSheet(R"(
        QTextEdit {
            background-color: #3a3a3a;
            border: 1px solid #444;
            border-radius: 8px;
        }
    )");
    mainLayout->addWidget(messageInput);

    // ----------------- Language Model Selection Widget -----------------
    modelSelectionWidget = new QWidget(this);
    modelSelectionWidget->setStyleSheet(R"(
        QWidget {
            background-color: #2b2b2b;
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
    // Add stretch to push buttons to the right
    buttonLayout->addStretch();

    attachButton = new QPushButton("📎", this);
    attachButton->setMinimumHeight(48);
    attachButton->setMinimumWidth(100);
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
    connect(attachButton, &QPushButton::clicked, this, &ChatPanelWidget::onAttachClicked);
    buttonLayout->addWidget(attachButton);

    toolsButton = new QPushButton("Tools ▼", this);
    toolsButton->setMinimumHeight(48);
    toolsButton->setMinimumWidth(100);
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
    sendButton->setMinimumHeight(48);
    sendButton->setMinimumWidth(100);
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
    //buttonLayout->addStretch(); // optional, push buttons to left

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
        QTimer::singleShot(10, this, [message, this]() {
            QString content = message->content();
            if (content.startsWith("[") && content.endsWith("]")) {
                content = "```json" + content + "```";
            }
            chatWidget->setMessage(message->content(), message->role() == ChatMessage::ChatRole);
        });
    });
    connect(chatModel, &ChatModel::messageRemoved, this, [](int index) { //
        qDebug().noquote() << "[CHATWIDGET] messageRemoved row:" << index;
    });
    connect(chatModel, &ChatModel::messageChanged, this, [](int index, ChatMessage *message) { //
        qDebug().noquote() << "[CHATWIDGET] messageChanged row:" << index << message->id();
    });

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

    // Emit or handle attached file in network layer
    qDebug() << "File attached:" << file;

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
