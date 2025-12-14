#include <llmchatclient.h>
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
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &LLMChatClient::onLLMResponse);
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

inline void LLMChatClient::reportError(const QString &message)
{
    qCritical("[LLMChatClient] ERROR: %s", qPrintable(message));

    emit errorOccurred("Server URL not set");
}

inline void LLMChatClient::sendRequest(const QJsonObject &requestBody, const QString &endpoint, bool isGetMethod)
{
    if (m_serverUrl.isEmpty()) {
        reportError("Server URL not set");
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
        this->onLLMResponse(qobject_cast<QNetworkReply *>(sender()));
    });

    connect(reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError error) { //
        this->onError(error);
    });

    connect(reply, &QNetworkReply::sslErrors, this, [this](const QList<QSslError> &errors) { //
        this->onSslErrors(qobject_cast<QNetworkReply *>(sender()), errors);
    });
#if 0
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
#endif
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
#if 0
    qDebug().noquote() << "[LLMChatClient] buildChatCompletionRequest" //
                       << "stream:" << stream                          //
                       << "model:" << model                            //
                       << "json:" << messages                          //
                       << "params:" << parameters;
#endif
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

// =========================================================

void LLMChatClient::onError(QNetworkReply::NetworkError error)
{
    qCritical().noquote() << "[LLMChatClient] onError reply:" << error;
    emit networkError(error, "Network error occurred");
}

void LLMChatClient::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    qCritical().noquote() << "[LLMChatClient] onSslErrors reply:" << reply << "ssl:" << errors;
    emit networkError(QNetworkReply::ProtocolUnknownError, "Network error occurred");
    // Handle SSL errors
    for (const QSslError &error : errors) {
        reportError("SSL Error: " + error.errorString());
    }
}

void LLMChatClient::onLLMResponse(QNetworkReply *reply)
{
    qDebug().noquote() << "[LLMChatClient] onLLMResponse --------------------------";

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

    // Check if a streamed LLM message
    if (data.startsWith("data:") || m_isResponseStream) {
        qDebug().noquote() << "[LLMChatClient] onFinished -> stream message.";
        parseResponse(data);
        m_isResponseStream = true;
        goto finish;
    }

    // Check if a regular LLM message
    doc = QJsonDocument::fromJson(data, &error);
    if (doc.isNull() || error.error != QJsonParseError::NoError) {
        reportError(tr("Invalid LLM response: %1").arg(error.errorString()));
        goto finish;
    }

    // get responsed JSON object
    response = doc.object();

    // Handle available models response
    if (reply->url().path().contains("/models")) {
        llmModels()->loadFrom(response["data"].toArray());
        qDebug("[LLMChatClient] onLLMResponse: %d LLM models available.", llmModels()->rowCount());
    }
    // Handle regular LLM message
    else {
        parseResponse(response);
    }

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

        parseResponse(doc.object());
    }
}

static inline QByteArray jsonTypeToString(QJsonValue::Type type)
{
    switch (type) {
        case QJsonValue::Undefined:
            return "Undefined";
        case QJsonValue::Null:
            return "Null";
        case QJsonValue::Bool:
            return "Boolean";
        case QJsonValue::Double:
            return "Double";
        case QJsonValue::String:
            return "String";
        case QJsonValue::Array:
            return "Array";
        case QJsonValue::Object:
            return "Object";
        default:
            return "Unknown Type";
    }
}

inline bool LLMChatClient::validateValue(const QJsonValue &value, const QString &key, const QJsonValue::Type expectedType)
{
    const QByteArray msg1 = "[LLMChatClient] Invalid LLM reply message. Field '%1' is missing.";
    const QByteArray msg2 = "[LLMChatClient] Field '%1' must not be NULL or empty.";
    const QByteArray msg3 = "[LLMChatClient] Field '%1' must be of type %2.";
    if (value.isUndefined()) {
        reportError(tr(msg1).arg(key)); // Key is missing
        return false;
    }
    if (value.isNull()) {
        reportError(tr(msg2).arg(key)); // Value is NULL
        return false;
    }
    if (value.type() != expectedType) {
        reportError(tr(msg3).arg(key).arg(jsonTypeToString(expectedType))); // Type mismatch
        return false;
    }
    return true;
}

inline bool LLMChatClient::valueOf(const QJsonObject &response, const QString &key, const QJsonValue::Type expectedType, QJsonValue &value)
{
    value = response.value(key);
    return validateValue(value, key, expectedType);
}

inline bool LLMChatClient::parseToolCall(const QJsonObject toolObject, ChatMessage::ToolEntry &tool) const
{
    QJsonValue value;

    // Extract type - may empty basedd on streamed message
    value = toolObject["type"];
    if (!value.isNull() && value.isString()) {
        tool.setToolType(value.toString());
    }

    // Extract id - may empty basedd on streamed message
    value = toolObject["id"];
    if (!value.isNull() && value.isString()) {
        tool.setToolCallId(value.toString());
    }

    // Extract index - may empty basedd on streamed message
    value = toolObject["index"];
    if (!value.isNull() && value.isDouble()) {
        tool.setToolIndex(static_cast<int>(value.toDouble()));
    }

    // Extract function information - may empty basedd on streamed message
    value = toolObject["function"];
    if (!value.isNull() && value.isObject()) {
        QJsonObject object = value.toObject();
        // Extract name - may empty basedd on streamed message
        value = object["name"];
        if (!value.isNull() && value.isString()) {
            tool.setFunctionName(value.toString());
        }
        // Extract arguments - may empty basedd on streamed message
        value = object["arguments"];
        if (!value.isNull() && value.isString()) {
            tool.setArguments(value.toString());
        }
    }

    return true;
}

inline bool LLMChatClient::parseToolCalls(ChatMessage *message, const QJsonArray &toolCalls)
{
    // May empty base on streamed message
    if (toolCalls.isEmpty()) {
        return true;
    }
    for (int i = 0; i < toolCalls.count(); i++) {
        if (toolCalls[i].isNull() || !toolCalls[i].isObject()) {
            reportError(tr("Invalid LLM reply message. Field 'tool_calls' is invalid."));
            continue;
        }
        ChatMessage::ToolEntry tool = {};
        QJsonObject toolObject = toolCalls[i].toObject();
        //qDebug().noquote() << "[LLMChatClient] parseToolCalls msgId:" << message->id() << "toolObject:" << toolObject;
        parseToolCall(toolObject, tool);
        message->mergeToolsFrom(tool);
    }
    return true;
}

inline bool LLMChatClient::parseChoiceObject(ChatMessage *message, const QJsonObject &choiceObject)
{
    //qDebug().noquote() << "[LLMChatClient] parseChoiceObject msgId:" << message->id() << "choice object:" << choiceObject;

    QJsonObject choice = choiceObject;
    QJsonValue value;

    // Extract stream delta message object if exist
    // 'message' and 'delta' are mutual exclusive!
    if (choice.contains("delta")) {
        value = choice["delta"];
        if (value.isNull() || !value.isObject()) {
            reportError(tr("Invalid LLM reply message. Field 'delta' is invalid."));
            return false;
        }
        // Set delta object as regular 'message' object and parse again
        choice["message"] = value.toObject();
    }

    // Extract finish_reason - may empty or null or not exist
    if (choice.contains("finish_reason")) {
        value = choice["finish_reason"];
        if (!value.isNull() && value.isString()) {
            message->setFinishReason(value.toString());
        }
    }

    // Extract index - may empty or null or not exist
    if (choice.contains("index")) {
        value = choice["index"];
        if (!value.isNull() && value.isString()) {
            message->setChoiceIndex(static_cast<int>(value.toDouble()));
        }
    }

    // normale message or tool call item - must be exist!
    value = choice["message"];
    if (value.isNull() || !value.isObject()) {
        reportError(tr("Invalid LLM reply message. Field 'message or delta' is invalid."));
        return false;
    }

    // get message object
    QJsonObject messageObj = value.toObject();

    // Extract content if exist
    if (messageObj.contains("content")) {
        value = messageObj["content"];
        if (!value.isNull() && value.isString()) {
            message->appendContent(value.toString());
        }
    }

    // Extract message role
    if (messageObj.contains("role")) {
        value = messageObj["role"];
        if (!value.isNull() && value.isString()) {
            QString roleStr = value.toString().toLower().trimmed();
            if (roleStr == "assistant") {
                message->setRole(ChatMessage::AssistantRole);
            } else if (roleStr == "user") {
                message->setRole(ChatMessage::UserRole);
            } else if (roleStr == "system") {
                message->setRole(ChatMessage::SystemRole);
            } else {
                message->setRole(ChatMessage::AssistantRole); // Default
            }
        } else if (message->role() == ChatMessage::NoRole) {
            message->setRole(ChatMessage::AssistantRole); // Default
        }
    }

    // Extract tool_calls if exist.
    if (messageObj.contains("tool_calls")) {
        value = messageObj["tool_calls"];
        if (value.isNull() || !value.isArray()) {
            reportError(tr("Invalid LLM reply message. Field 'tool_calls' is invalid."));
            return false;
        }
        QJsonArray tools = value.toArray();
        if (!tools.isEmpty()) {
            return parseToolCalls(message, tools);
        }
    }

    return true;
}

inline bool LLMChatClient::parseChoices(ChatMessage *message, const QJsonArray &choices)
{
    // can be empty on 'stop' or 'tools_call' condition
    if (choices.isEmpty()) {
        return true;
    }
    for (int i = 0; i < choices.count(); i++) {
        QJsonValue value = choices.at(i);
        // skip invalid entries
        if (!validateValue(value, QStringLiteral("choices[%1]").arg(i), QJsonValue::Type::Object)) {
            reportError("Invalid LLM reply message. Choices element must be of type object.");
            continue;
        }
        if (!parseChoiceObject(message, value.toObject())) {
            return false;
        }
    }
    return true;
}

inline void LLMChatClient::parseResponse(const QJsonObject &response)
{
    //qDebug().noquote() << "[LLMChatClient] parseResponse object:" << response;

    bool isNew = false;
    ChatMessage *message = 0L;
    QJsonValue value;

    // handle chat message response
    if (!valueOf(response, "id", QJsonValue::Type::String, value)) {
        return;
    }

    // take existing message or create new one if not exist
    message = m_chatModel->messageById(value.toString());
    if (message == nullptr) {
        message = new ChatMessage(m_chatModel);
        isNew = true;
    }
    message->setId(value.toString());

    if (!valueOf(response, "object", QJsonValue::Type::String, value)) {
        goto error_exit;
    }
    message->setObject(value.toString());

    if (!valueOf(response, "created", QJsonValue::Type::Double, value)) {
        goto error_exit;
    }
    message->setCreated(static_cast<qint64>(value.toDouble()));

    if (!valueOf(response, "model", QJsonValue::Type::String, value)) {
        goto error_exit;
    }
    message->setModel(value.toString());

    if (!valueOf(response, "system_fingerprint", QJsonValue::Type::String, value)) {
        goto error_exit;
    }
    message->setSystemFingerprint(value.toString());

    // optional header fields
    if (response.contains("stats")) {
        if (!valueOf(response, "stats", QJsonValue::Type::Object, value)) {
            goto error_exit;
        }
        message->setStats(value.toObject());
    }
    if (response.contains("usage")) {
        if (!valueOf(response, "usage", QJsonValue::Type::Object, value)) {
            goto error_exit;
        }
        message->setUsage(value.toObject());
    }

    if (!valueOf(response, "choices", QJsonValue::Type::Array, value)) {
        goto error_exit;
    }
    if (!parseChoices(message, value.toArray())) {
        goto error_exit;
    }

    if (isNew) {
        chatModel()->addMessage(message);
    }

    // run tooling (tool_calls)
    if (message->finishReason().toLower().trimmed() == "tool_calls") {
        checkAndRunTooling(message);
    }

    // conversation stopped
    if (message->finishReason().toLower().trimmed() == "stop") {
        emit streamCompleted();
    }

    // sueccess
    return;

error_exit:
    if (message) {
        message->deleteLater();
    }
}

inline void LLMChatClient::checkAndRunTooling(ChatMessage *message)
{
    // ToolService: execute tool through MCP or SDIO or onboard
    foreach (const ChatMessage::ToolEntry &tool, message->tools()) {
        // Check for an incomplete tool object based on a streamed response
        if (!tool.isValid()) {
            qWarning("[LLMChatClient] checkAndRunTooling msgId: %s Invalid tool object in message.", //
                     qPrintable(message->id()));
            continue;
        }
        qDebug("[LLMChatClient] checkAndRunTooling msgId: %s tool[%d] type=%s id=%s function=%s args=%s", //
               qPrintable(message->id()),
               tool.toolIndex(),
               qPrintable(tool.m_toolType),
               qPrintable(tool.m_toolCallId),
               qPrintable(tool.m_functionName),
               qPrintable(tool.m_arguments));
        emit toolRequest(message, tool);
    }
}

void LLMChatClient::cancelRequest()
{
    m_networkManager->clearAccessCache();
    m_networkManager->clearConnectionCache();
    emit streamCompleted();
}

void LLMChatClient::setActiveModel(const ModelEntry &model)
{
    m_llmModel = model;
}
