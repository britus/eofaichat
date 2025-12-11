#ifndef LLMCHAT_CLIENT_H
#define LLMCHAT_CLIENT_H
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
    explicit LLMChatClient(QObject *parent = nullptr);

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

    ToolModel *toolModel() const;

public slots:
    void setToolModel(ToolModel *newToolModel);

signals:
    // Success signals
    void chatCompletionReceived(const QJsonObject &response);
    void modelListReceived(const QJsonArray &models);
    void streamingDataReceived(const QString &data);

    // Error signals
    void errorOccurred(const QString &error);
    void networkError(QNetworkReply::NetworkError error, const QString &message);

private slots:
    void onFinished(QNetworkReply *reply);
    void onError(QNetworkReply::NetworkError error);
    void onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_serverUrl;
    QString m_apiKey;
    int m_timeout;
    ToolModel *m_toolModel;

private:
    void sendRequest(const QJsonObject &requestBody, const QString &endpoint, bool isGetMethod = false);
    QJsonObject buildChatCompletionRequest(const QString &model, const QList<QJsonObject> &messages, const QJsonObject &parameters, bool stream);
    void parseStreamingResponse(QNetworkReply *reply);
    QJsonArray loadToolsConfig() const;
};

#endif // LLMCHAT_CLIENT_H
