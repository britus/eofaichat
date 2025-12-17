#include <llmchatclient.h>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrlQuery>

LLMChatClient::LLMChatClient(ToolModel *toolModel, QObject *parent)
    : QObject(parent)
    , m_toolModel(toolModel)
    , m_llmModels(new ModelListModel(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_connection(nullptr)
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

void LLMChatClient::setConnection(LLMConnection *connection)
{
    m_connection = connection;
}

void LLMChatClient::setTimeout(int milliseconds)
{
    m_timeout = milliseconds;
}

/*
Sample of conversation request with tooling openAI -like:
{
  "messages": [
    {
      "role": "user",
      "content": "What is the current state of AI development?"
    },
    {
      "role": "assistant",
      "content": "AI development has made tremendous progress in recent years, especially in natural language processing and computer vision..."
    },
    {
      "role": "tool",
      "toolName": "WebSearch",
      "parameters": {
        "query": "current trends in AI development"
      },
      "result": {
        "content": "Current trends include increasing efficiency of AI models as well as a focus on ethical aspects..."
      }
    },
    {
      "role": "assistant",
      "content": "According to the latest trends, AI development is focusing on efficiency and ethical challenges..."
    }
  ]
}
*/

void LLMChatClient::sendChat(const QList<SendParameters> &parameters, bool stream, int maxTokens, double temperature)
{
    // Create list with single message
    QList<QJsonObject> messages;

    foreach (auto &p, parameters) {
        // Create a single message object
        QJsonObject messageObj;
        switch (p.role) {
            case ChatMessage::NoRole: {
                messageObj["role"] = "user";
                break;
            }
            case ChatMessage::Role::AssistantRole: {
                messageObj["role"] = "assistant";
                break;
            }
            case ChatMessage::UserRole: {
                messageObj["role"] = "user";
                break;
            }
            case ChatMessage::SystemRole: {
                messageObj["role"] = "system";
                break;
            }
            case ChatMessage::ChatRole: {
                messageObj["role"] = "user";
                break;
            }
            case ChatMessage::ToolingRole: {
                messageObj["role"] = "tool"; //tooling
                break;
            }
            case ChatMessage::LlmRole: {
                messageObj["role"] = "llm";
                break;
            }
        }

        // Standard user text
        messageObj["content"] = p.content.isEmpty() ? p.toolResult : p.content;

        if (!p.toolName.isEmpty()) {
            messageObj["tool_name"] = p.toolName;
            messageObj["parameters"] = QJsonObject{
                QPair<QString, QString>("query", p.toolQuery),
            };
            messageObj["result"] = QJsonObject{
                QPair<QString, QString>("content", p.toolResult),
            };
        }

        messages.append(messageObj);
    }

    // Call the main createChatCompletion method
    sendChat(activeModel().id, messages, stream, maxTokens, temperature);
}

void LLMChatClient::sendChat(const SendParameters &parameter, bool stream, int maxTokens, double temperature)
{
    // Create list with single message
    QList<SendParameters> parameters;
    parameters.append(parameter);

    sendChat(parameters, stream, maxTokens, temperature);
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
    if (m_connection && m_connection->isValid()) {
        QJsonObject requestBody = buildChatCompletionRequest(model, messages, parameters, stream);
        sendRequest(requestBody,
                    m_connection->endpointUri( //
                        LLMConnection::EndpointCompletion));
    }
}

void LLMChatClient::cancelRequest()
{
    m_networkManager->clearAccessCache();
    m_networkManager->clearConnectionCache();
}

void LLMChatClient::setActiveModel(const ModelListModel::ModelEntry &model)
{
    m_llmModel = model;
}

void LLMChatClient::listModels()
{
    if (m_connection && m_connection->isValid()) {
        QJsonObject requestBody;
        sendRequest(requestBody,
                    m_connection->endpointUri( //
                        LLMConnection::EndpointModels),
                    true);
    }
}

inline void LLMChatClient::reportError(const QString &message)
{
    qCritical("[LLMChatClient] ERROR: %s", qPrintable(message));

    emit errorOccurred("Server URL not set");
}

inline void LLMChatClient::sendRequest(const QJsonObject &requestBody, const QString &endpoint, bool isGetMethod)
{
    if (!m_connection || !m_connection->isValid()) {
        reportError("Server URL not set");
        return;
    }

    QString apiUrl = m_connection->apiUrl();
    if (!apiUrl.endsWith('/') && !endpoint.startsWith("/")) {
        apiUrl + "/" + endpoint;
    } else if (apiUrl.endsWith("/") && endpoint.startsWith("/")) {
        apiUrl = apiUrl + endpoint.mid(1);
    } else {
        apiUrl = apiUrl + endpoint;
    }

    QUrl url(apiUrl);
    QNetworkRequest request(url);

    request.setTransferTimeout(m_timeout);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_connection->apiKey().isEmpty()) {
        switch (m_connection->authType()) {
            case LLMConnection::AuthType::AuthToken: {
                request.setRawHeader("Authorization", "Token " + m_connection->apiKey().toUtf8());
                break;
            }
            case LLMConnection::AuthType::AuthBearer: {
                request.setRawHeader("Authorization", "Bearer " + m_connection->apiKey().toUtf8());
                break;
            }
        }
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
        goto finish;
    }

    data = reply->readAll();
    if (data.length() == 0) {
        goto finish;
    }

    // Check if a streamed LLM message
    if (data.startsWith("data:") || m_isResponseStream) {
        qDebug().noquote() << "[LLMChatClient] onFinished -> stream message.";
        parseDataStream(data);
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
        modelList()->loadFrom(response["data"].toArray());
        qDebug("[LLMChatClient] onLLMResponse: %d LLM models available.", modelList()->rowCount());
    }
    // Handle regular LLM message
    else {
        parseDataObject(response);
    }

finish:
    reply->deleteLater();
}
