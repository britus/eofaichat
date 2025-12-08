#include "llmchatclient.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QUrlQuery>

LLMChatClient::LLMChatClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_timeout(30000) // 30 seconds default
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

void LLMChatClient::sendRequest(const QJsonObject &requestBody, const QString &endpoint, bool isGetMethod)
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

    QNetworkReply *reply;
    if (isGetMethod) {
        reply = m_networkManager->get(request);
    } else {
        QJsonDocument doc(requestBody);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        reply = m_networkManager->post(request, data);
    }

    // Connect all relevant signals to the reply
    connect(reply, &QNetworkReply::finished, [reply, this]() { //
        this->onFinished(reply);
    });

    connect(reply, &QNetworkReply::errorOccurred, [this](QNetworkReply::NetworkError error) { //
        this->onError(error);
    });

    connect(reply, &QNetworkReply::sslErrors, [reply, this](const QList<QSslError> &errors) { //
        this->onSslErrors(reply, errors);
    });

    connect(reply, &QNetworkReply::downloadProgress, [/*reply, this*/](qint64 bytesReceived, qint64 bytesTotal) { //
        qDebug() << "[LLMCC] downloadProgress:" << bytesReceived << "/" << bytesTotal;
        // Handle download progress if needed
    });

    connect(reply, &QNetworkReply::uploadProgress, [/*reply, this*/](qint64 bytesSent, qint64 bytesTotal) { //
        qDebug() << "[LLMCC] uploadProgress:" << bytesSent << "/" << bytesTotal;
        // Handle upload progress if needed
    });

    connect(reply, &QNetworkReply::metaDataChanged, [/*reply, this*/]() { //
        qDebug() << "[LLMCC] metaDataChanged";
        // Handle metadata changes if needed
    });
}

QJsonObject LLMChatClient::buildChatCompletionRequest(const QString &model, const QList<QJsonObject> &messages, const QJsonObject &parameters, bool stream)
{
    qDebug().noquote() << "[LLMCC] buildChatCompletionRequest" //
                       << "stream:" << stream                  //
                       << "model:" << model                    //
                       << "json:" << messages                  //
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

    // Streaming
    request["stream"] = stream;

    return request;
}

void LLMChatClient::onFinished(QNetworkReply *reply)
{
    qDebug().noquote() << "[LLMCC] onFinished reply:" << reply;

    if (reply->error() != QNetworkReply::NoError) {
        emit networkError(reply->error(), reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        qDebug().noquote() << "[LLMCC] onFinished no response";
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull()) {
        qDebug().noquote() << "[LLMCC] onFinished JSON document is empty";
        emit errorOccurred("Invalid JSON response");
        reply->deleteLater();
        return;
    }

    QJsonObject response = doc.object();

    // Check if it's a streaming response
    if (response.contains("choices")                //
        && response["choices"].toArray().size() > 0 //
        && response["choices"].toArray()[0].toObject().contains("delta")) {
        // This is a streaming response - handle specially
        parseStreamingResponse(reply);
    } else {
        // Regular response
        if (reply->url().path().contains("/models")) {
            emit modelListReceived(response["data"].toArray());
        } else {
            emit chatCompletionReceived(response);
        }
    }

    reply->deleteLater();
}

void LLMChatClient::parseStreamingResponse(QNetworkReply *reply)
{
    qDebug().noquote() << "[LLMCC] parseStreamingResponse reply:" << reply;

    // For streaming responses, we need to parse line-by-line
    QByteArray data = reply->readAll();
    QString responseStr(data);

    // Split by newlines and process each line
    QStringList lines = responseStr.split('\n');

    foreach (const QString &line, lines) {
        if (line.startsWith("data: ")) {
            QString dataStr = line.mid(6); // Remove "data: "

            if (dataStr.trimmed() == "[DONE]") {
                // End of stream
                continue;
            }

            QJsonDocument doc = QJsonDocument::fromJson(dataStr.toUtf8());
            if (!doc.isNull()) {
                emit streamingDataReceived(dataStr);
            }
        }
    }
}

void LLMChatClient::onError(QNetworkReply::NetworkError error)
{
    qDebug().noquote() << "[LLMCC] onError reply:" << error;

    emit networkError(error, "Network error occurred");
}

void LLMChatClient::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    qDebug().noquote() << "[LLMCC] onSslErrors reply:" << reply << "ssl:" << errors;

    emit networkError(QNetworkReply::ProtocolUnknownError, "Network error occurred");

    // Handle SSL errors
    for (const QSslError &error : errors) {
        emit errorOccurred("SSL Error: " + error.errorString());
    }
}
