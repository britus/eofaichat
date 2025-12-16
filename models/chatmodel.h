#ifndef CHATMODEL_H
#define CHATMODEL_H

#include <chatmessage.h>
#include <QAbstractListModel>
#include <QList>

class ChatModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        ContentRole = Qt::UserRole + 1,
        RoleRole,
        CreatedRole,
        IdRole,
        ModelRole,
        ObjectRole,
        SystemFingerprintRole,
    };

    explicit ChatModel(QObject *parent = nullptr);

    // Model interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Custom methods
    void clear();
    ChatMessage *addMessage(const ChatMessage &message);
    ChatMessage *addMessage(ChatMessage *message);
    void removeMessage(int index);

    ChatMessage *messageAt(int index) const;
    ChatMessage *messageById(const QString &id);

    // Save and load methods
    bool saveToFile(const QString &fileName) const;
    bool loadFromFile(const QString &fileName);

public slots:
    void onParseMessageObject(const QJsonObject &response);
    void onParseDataStream(const QByteArray &data);

signals:
    void streamCompleted();
    void errorOccurred(const QString &error);
    void toolRequest(ChatMessage *message, const ToolCallEntry &tool);
    void messageAdded(ChatMessage *message);
    void messageRemoved(int index);
    void messageChanged(int index, ChatMessage *message);

private:
    QList<ChatMessage *> m_messages;

private:
    inline void reportError(const QString &message);
    inline bool validateValue(const QJsonValue &value, const QString &key, const QJsonValue::Type expectedType);
    inline bool valueOf(const QJsonObject &response, const QString &key, const QJsonValue::Type expectedType, QJsonValue &value);
    inline bool parseChoices(ChatMessage *message, const QJsonArray &choices);
    inline bool parseChoiceObject(ChatMessage *message, const QJsonObject &choice);
    inline bool parseToolCalls(ChatMessage *message, const QJsonArray &toolCalls);
    inline bool parseToolCall(const QJsonObject toolObject, ToolCallEntry &tool) const;
    inline void checkAndRunTooling(ChatMessage *messge);
};

#endif // CHATMODEL_H
