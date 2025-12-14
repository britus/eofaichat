#ifndef LLMCHAT_CLIENT_H
#define LLMCHAT_CLIENT_H
#include <chatmessage.h>
#include <chatmodel.h>
#include <modellistmodel.h>
#include <toolmodel.h>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QTimer>
#include <QUrl>

class LLMChatClient : public QObject
{
    Q_OBJECT

public:
    explicit LLMChatClient(ToolModel *toolModel, ChatModel *chatModel, QObject *parent = nullptr);

    ~LLMChatClient();

    // Configuration methods
    void setTimeout(int milliseconds);
    void setServerUrl(const QString &url);
    void setApiKey(const QString &key);

    // Convenience method for single string message
    void sendChat(const QString &model, const QString &message, bool stream = false, int maxTokens = 65536, double temperature = 0.7);

    // Chat completion methods
    void sendChat(const QString &model, const QList<QJsonObject> &messages, bool stream = false, int maxTokens = 65536, double temperature = 0.7);

    // Chat completion with parameters
    void sendChat(const QString &model, const QList<QJsonObject> &messages, const QJsonObject &parameters, bool stream = false);

    // Model listing
    void listModels();

    inline ToolModel *toolModel() { return m_toolModel; }
    inline ChatModel *chatModel() { return m_chatModel; }
    inline ModelListModel *llmModels() { return m_llmModels; }
    inline const ModelEntry &activeModel() const { return m_llmModel; }

public slots:
    void setActiveModel(const ModelEntry &model);
    void cancelRequest();

signals:
    void errorOccurred(const QString &error);
    void networkError(QNetworkReply::NetworkError error, const QString &message);
    void streamCompleted();
    void toolRequest(ChatMessage *message, const ChatMessage::ToolEntry &tool);

private slots:
    void onLLMResponse(QNetworkReply *reply);
    void onError(QNetworkReply::NetworkError error);
    void onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    ModelEntry m_llmModel;
    ToolModel *m_toolModel;
    ChatModel *m_chatModel;
    ModelListModel *m_llmModels;
    QNetworkAccessManager *m_networkManager;
    QString m_serverUrl;
    QString m_apiKey;
    int m_timeout;
    bool m_isResponseStream;

private:
    inline void reportError(const QString &message);
    inline void sendRequest(const QJsonObject &requestBody, const QString &endpoint, bool isGetMethod = false);
    inline QJsonObject buildChatCompletionRequest(const QString &model, const QList<QJsonObject> &messages, const QJsonObject &parameters, bool stream);
    inline bool validateValue(const QJsonValue &value, const QString &key, const QJsonValue::Type expectedType);
    inline bool valueOf(const QJsonObject &response, const QString &key, const QJsonValue::Type expectedType, QJsonValue &value);
    inline void parseResponse(const QJsonObject &response);
    inline void parseResponse(const QByteArray &data);
    inline bool parseChoices(ChatMessage *message, const QJsonArray &choices);
    inline bool parseChoiceObject(ChatMessage *message, const QJsonObject &choice);
    inline bool parseToolCalls(ChatMessage *message, const QJsonArray &toolCalls);
    inline bool parseToolCall(const QJsonObject toolObject, ChatMessage::ToolEntry &tool) const;
    inline QJsonArray loadToolsConfig() const;
    inline void checkAndRunTooling(ChatMessage *messge);
};

#endif // LLMCHAT_CLIENT_H
