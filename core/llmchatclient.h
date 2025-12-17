#ifndef LLMCHAT_CLIENT_H
#define LLMCHAT_CLIENT_H
#include <chatmessage.h>
#include <chatmodel.h>
#include <llmconnectionmodel.h>
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
#include <QString>
#include <QTimer>
#include <QUrl>

class LLMChatClient : public QObject
{
    Q_OBJECT

public:
    struct SendParameters
    {
        ChatMessage::Role role = ChatMessage::Role::UserRole;
        QString content;
        QString toolName;
        QString toolQuery; // user question
        QString toolResult;
    };

    explicit LLMChatClient(ToolModel *toolModel, QObject *parent = nullptr);

    ~LLMChatClient();

    // Configuration methods
    void setTimeout(int milliseconds);
    void setConnection(LLMConnection *connection);

    // Convenience method for single string message

    void sendChat(const QList<SendParameters> &parameters, bool stream = false, int maxTokens = 65536, double temperature = 0.7);
    void sendChat(const SendParameters &parameters, bool stream = false, int maxTokens = 65536, double temperature = 0.7);

    // Model listing
    void listModels();

    inline ToolModel *toolModel() { return m_toolModel; }
    inline ModelListModel *modelList() const { return m_llmModels; }
    inline const ModelListModel::ModelEntry &activeModel() const { return m_llmModel; }
    inline bool hasLLModels() const { return m_llmModels && m_llmModels->rowCount() > 0; }
    inline bool modelCount() const { return (m_llmModels ? m_llmModels->rowCount() : 0); }

public slots:
    void setActiveModel(const ModelListModel::ModelEntry &model);
    void cancelRequest();

signals:
    void errorOccurred(const QString &error);
    void networkError(QNetworkReply::NetworkError error, const QString &message);
    void parseDataStream(const QByteArray &data);
    void parseDataObject(const QJsonObject &response);

protected:
    // Chat completion methods
    void sendChat(const QString &model, const QList<QJsonObject> &messages, bool stream = false, int maxTokens = 65536, double temperature = 0.7);
    // Chat completion with parameters
    void sendChat(const QString &model, const QList<QJsonObject> &messages, const QJsonObject &parameters, bool stream = false);

private slots:
    void onLLMResponse(QNetworkReply *reply);
    void onError(QNetworkReply::NetworkError error);
    void onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    QNetworkAccessManager *m_networkManager;
    // tool support
    ToolModel *m_toolModel;
    // all available LL models
    ModelListModel *m_llmModels;
    // selected (avtive) model
    ModelListModel::ModelEntry m_llmModel;
    // avtivated LLM server connection to use
    LLMConnection *m_connection;
    // configured time out
    int m_timeout;
    // True if server response is a stream
    bool m_isResponseStream;

private:
    inline void reportError(const QString &message);
    inline void sendRequest(const QJsonObject &requestBody, const QString &endpoint, bool isGetMethod = false);
    inline QJsonArray loadToolsConfig() const;
    inline QJsonObject buildChatCompletionRequest(const QString &model, const QList<QJsonObject> &messages, const QJsonObject &parameters, bool stream);
};

#endif // LLMCHAT_CLIENT_H
