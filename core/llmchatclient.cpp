#include <llmchatclient.h>
#include <toolservice.h>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrlQuery>

LLMChatClient::LLMChatClient(ToolModel *toolModel, ChatModel *chatModel, QObject *parent)
    : QObject(parent)
    , m_toolModel(toolModel)
    , m_chatModel(chatModel)
    , m_llmModels(new ModelListModel(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_timeout(60000) // 1m default
    , m_isResponseStream(false)
{
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &LLMChatClient::onFinished);
    connect(m_networkManager, &QNetworkAccessManager::sslErrors, this, &LLMChatClient::onSslErrors);
}

LLMChatClient::~LLMChatClient()
{
    if (m_networkManager) {
        m_networkManager->deleteLater();
    }
}

void LLMChatClient::setServerUrl(const QString &url)
{
    m_serverUrl = url;
    if (!m_serverUrl.endsWith('/')) {
        m_serverUrl.append('/');
    }
}

void LLMChatClient::setApiKey(const QString &key)
{
    m_apiKey = key;
}

void LLMChatClient::setTimeout(int milliseconds)
{
    m_timeout = milliseconds;
}

void LLMChatClient::sendChat(const QString &model, const QString &message, bool stream, int maxTokens, double temperature)
{
    // Create a single message object
    QJsonObject messageObj;
    messageObj["role"] = "user";
    messageObj["content"] = message;

    // Create list with single message
    QList<QJsonObject> messages;
    messages.append(messageObj);

    // Call the main createChatCompletion method
    sendChat(model, messages, stream, maxTokens, temperature);
}

void LLMChatClient::sendChat(const QString &model, const QList<QJsonObject> &messages, bool stream, int maxTokens, double temperature)
{
    QJsonObject parameters;
    parameters["max_tokens"] = maxTokens;
    parameters["temperature"] = temperature;

    sendChat(model, messages, parameters, stream);
}

void LLMChatClient::sendChat(const QString &model, const QList<QJsonObject> &messages, const QJsonObject &parameters, bool stream)
{
    QJsonObject requestBody = buildChatCompletionRequest(model, messages, parameters, stream);
    sendRequest(requestBody, "v1/chat/completions");
}

void LLMChatClient::listModels()
{
    QJsonObject requestBody;
    sendRequest(requestBody, "v1/models", true);
}

inline void LLMChatClient::sendRequest(const QJsonObject &requestBody, const QString &endpoint, bool isGetMethod)
{
    if (m_serverUrl.isEmpty()) {
        emit errorOccurred("Server URL not set");
        return;
    }

    QUrl url(m_serverUrl + endpoint);
    QNetworkRequest request(url);

    request.setTransferTimeout(m_timeout);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
        //request.setRawHeader("Authorization", "Token " + m_apiKey.toUtf8());
    }

    // ensure to handle regular LLM messages
    m_isResponseStream = false;

    QNetworkReply *reply;
    if (isGetMethod) {
        reply = m_networkManager->get(request);
    } else {
        QJsonDocument doc(requestBody);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        reply = m_networkManager->post(request, data);
    }

    connect(reply, &QNetworkReply::finished, this, [this]() { //
        this->onFinished(qobject_cast<QNetworkReply *>(sender()));
    });

    connect(reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError error) { //
        this->onError(error);
    });

    connect(reply, &QNetworkReply::sslErrors, this, [this](const QList<QSslError> &errors) { //
        this->onSslErrors(qobject_cast<QNetworkReply *>(sender()), errors);
    });

    // Handle download progress if needed
    connect(reply, &QNetworkReply::downloadProgress, this, [/*this*/](qint64 bytesReceived, qint64 bytesTotal) { //
        qDebug() << "[LLMChatClient] downloadProgress:" << bytesReceived << "/" << bytesTotal;
    });

    // Handle download progress if needed
    connect(reply, &QNetworkReply::uploadProgress, this, [/*this*/](qint64 bytesSent, qint64 bytesTotal) { //
        qDebug() << "[LLMChatClient] uploadProgress:" << bytesSent << "/" << bytesTotal;
    });

    // Handle metadata changes if needed
    connect(reply, &QNetworkReply::metaDataChanged, this, [/*this*/]() { //
        qDebug() << "[LLMChatClient] metaDataChanged";
    });
}

inline QJsonArray LLMChatClient::loadToolsConfig() const
{
    if (!m_toolModel || m_toolModel->rowCount() == 0) {
        return QJsonArray();
    }

    QJsonArray result;
    QList<QJsonObject> toolList = m_toolModel->toolObjects();
    foreach (auto tool, toolList) {
        QJsonObject fncItem;
        fncItem["type"] = "function";
        fncItem["function"] = tool;
        result.append(fncItem);
    }

    return result;
}

inline QJsonObject LLMChatClient::buildChatCompletionRequest( //
    const QString &model,
    const QList<QJsonObject> &messages,
    const QJsonObject &parameters,
    bool stream)
{
    qDebug().noquote() << "[LLMChatClient] buildChatCompletionRequest" //
                       << "stream:" << stream                          //
                       << "model:" << model                            //
                       << "json:" << messages                          //
                       << "params:" << parameters;

    QJsonObject request;

    // Required fields
    request["model"] = model;

    // Convert QList<QJsonObject> to QJsonArray
    QJsonArray messagesArray;
    for (const QJsonObject &message : messages) {
        messagesArray.append(message);
    }
    request["messages"] = messagesArray;

    // Optional parameters
    if (!parameters.isEmpty()) {
        for (const QString &key : parameters.keys()) {
            request[key] = parameters.value(key);
        }
    }

    // Tools -> hm, not work for normal text?
    request["tools"] = loadToolsConfig();

    // Resources?
    //request["resources"] = QJsonArray();

    // Prompt Templates?
    //request["prompts"] = QJsonArray();

    // Streaming
    request["stream"] = stream;

    return request;
}

void LLMChatClient::onFinished(QNetworkReply *reply)
{
    qDebug().noquote() << "[LLMChatClient] onFinished reply:" << reply;

    QJsonParseError error;
    QJsonDocument doc;
    QJsonObject response;
    QByteArray data;

    if (reply->error() != QNetworkReply::NoError) {
        emit networkError(reply->error(), reply->errorString());
        goto finish;
    }

    data = reply->readAll();
    if (data.length() == 0) {
        goto finish;
    }

    // Check if a streaming response
    if (data.startsWith("data:") || m_isResponseStream) {
        qDebug().noquote() << "[LLMChatClient] onFinished -> stream message.";
        parseResponse(data);
        m_isResponseStream = true;
        goto finish;
    }

    // Check if a regular LLM message
    doc = QJsonDocument::fromJson(data, &error);
    if (doc.isNull() || error.error != QJsonParseError::NoError) {
        emit errorOccurred(tr("[LLMChatClient] Invalid LLM response: %1").arg(error.errorString()));
        goto finish;
    }
    response = doc.object();

    // Handle available models response
    if (reply->url().path().contains("/models")) {
        llmModels()->loadFrom(response["data"].toArray());
        goto finish;
    }

    // Handle regular LLM message
    parseResponse(response);

finish:
    reply->deleteLater();
}

inline void LLMChatClient::parseResponse(const QByteArray &data)
{
    // Split by newlines and process each line
    const uint streamTagLen = QStringLiteral("data:").length();
    const QString responseStr(data);
    const QStringList lines = responseStr.split('\n');

    foreach (const QString &line, lines) {
        if (line.isEmpty()) {
            continue;
        }

        // Remove "data: "
        QString dataStr = line.mid(streamTagLen).trimmed();

        //qDebug().noquote() << "[LLMChatClient] parseResponse line:" << dataStr;

        // Is end of stream ?
        if (dataStr.trimmed() == "[DONE]") {
            emit streamCompleted();
            continue;
        }

        // test JSON forment completed
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(dataStr.toUtf8(), &error);
        if (doc.isNull() || error.error != QJsonParseError::NoError) {
            qWarning().noquote() << "[LLMChatClient] parseResponse error:" << error.errorString();
            continue;
        }

        //qDebug().noquote() << "[LLMChatClient] parseResponse json:" << doc.toJson(QJsonDocument::Indented);
        parseResponse(doc.object());
    }
}

inline void LLMChatClient::parseResponse(const QJsonObject &response)
{
    //qDebug().noquote() << "[LLMChatClient] parseResponse object:" << response;

    // handle chat message response
    if (!response.contains("id")) {
        emit errorOccurred(tr("[LLMChatClient] Invalid LLM response message. Field 'id' missing."));
        return;
    }
    if (!response.contains("object")) {
        emit errorOccurred(tr("[LLMChatClient] Invalid LLM response message. Field 'object' missing."));
        return;
    }
    if (!response.contains("created")) {
        emit errorOccurred(tr("[LLMChatClient] Invalid LLM response message. Field 'created' missing."));
        return;
    }
    if (!response.contains("model")) {
        emit errorOccurred(tr("[LLMChatClient] Invalid LLM response message. Field 'model' missing."));
        return;
    }
    if (!response.contains("system_fingerprint")) {
        emit errorOccurred(tr("[LLMChatClient] Invalid LLM response message. Field 'system_fingerprint' missing."));
        return;
    }
    if (!response.contains("choices") || response["choices"].isNull()) {
        emit errorOccurred(tr("[LLMChatClient] Invalid LLM response message. Field 'choices' missing."));
        return;
    }
    if (!response["choices"].isArray()) {
        emit errorOccurred(tr("[LLMChatClient] LLM field 'choices' must be an array."));
        return;
    }
    if (response["choices"].toArray().isEmpty()) {
        emit errorOccurred(tr("[LLMChatClient] LLM field 'choices' is empty."));
        return;
    }

    // parse LLM message object
    ChatMessage cm(response, this);
    ChatMessage *original;

    // if LLM message stream, update existing message
    if ((original = chatModel()->messageById(cm.id()))) {
        original->mergeMessage(&cm);
        checkAndRunTooling(original);
    } else {
        chatModel()->addMessage(cm);
        checkAndRunTooling(&cm);
    }
}

inline void LLMChatClient::checkAndRunTooling(ChatMessage *message)
{
    // ToolService: execute tool through MCP or SDIO or onboard
    if (message->finishReason() == "tool_calls") {
        QTimer::singleShot(10, this, [this, message]() { //
            foreach (const ChatMessage::ToolEntry &tool, message->tools()) {
                qDebug("[LLMChatClient] Tool call: type=%s id=%s function=%s args=%s", //
                       qPrintable(tool.m_toolType),
                       qPrintable(tool.m_toolCallId),
                       qPrintable(tool.m_functionName),
                       qPrintable(tool.m_arguments));
                onToolRequest(tool);
            }
        });
    }
}

void LLMChatClient::onError(QNetworkReply::NetworkError error)
{
    qDebug().noquote() << "[LLMChatClient] onError reply:" << error;
    emit networkError(error, "Network error occurred");
}

void LLMChatClient::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    qDebug().noquote() << "[LLMChatClient] onSslErrors reply:" << reply << "ssl:" << errors;
    emit networkError(QNetworkReply::ProtocolUnknownError, "Network error occurred");
    // Handle SSL errors
    for (const QSslError &error : errors) {
        emit errorOccurred("SSL Error: " + error.errorString());
    }
}

void LLMChatClient::setActiveModel(const ModelEntry &model)
{
    m_llmModel = model;
}

void LLMChatClient::onToolRequest(const ChatMessage::ToolEntry &tool)
{
    ToolService toolService(this);

    if (activeModel().id.isEmpty()) {
        qCritical().noquote() << "[LLMChatClient] No LLM is activated.";
        return;
    }

    qDebug().noquote() << "[LLMChatClient] onToolRequest type:" //
                       << tool.toolType()                       //
                       << "id:" << tool.toolCallId()            //
                       << "function:" << tool.functionName();

    QJsonObject result;
    switch (tool.toolType()) {
        case ChatMessage::ToolType::Tool:
        case ChatMessage::ToolType::Function: {
            result = toolService.execute(this->toolModel(), tool.functionName(), tool.arguments());
            break;
        }
        case ChatMessage::ToolType::Resuource: {
            result = toolService.execute(this->toolModel(), tool.functionName(), tool.arguments());
            break;
        }
        case ChatMessage::ToolType::Prompt: {
            result = toolService.execute(this->toolModel(), tool.functionName(), tool.arguments());
            break;
        }
    }
    if (!result.isEmpty()) {
        sendChat(activeModel().id, QJsonDocument(result).toJson(QJsonDocument::Compact), true);
    } else {
        const QJsonObject error = toolService.createErrorResponse( //
            QStringLiteral("Tool '%1' does not produce any results.").arg(tool.functionName()));
        sendChat(activeModel().id, QJsonDocument(error).toJson(QJsonDocument::Compact), true);
    }

    emit toolCompleted(tool);
}
