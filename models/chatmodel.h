#ifndef CHATMODEL_H
#define CHATMODEL_H

#include "chatmessage.h"
#include <QAbstractListModel>
#include <QList>

class ChatModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles { ContentRole = Qt::UserRole + 1, RoleRole, CreatedRole, IdRole, ModelRole, ObjectRole, SystemFingerprintRole };

    explicit ChatModel(QObject *parent = nullptr);

    // Model interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Custom methods
    void clear();
    void addMessage(const ChatMessage &message);
    void addMessageFromJson(const QJsonObject &json);
    void removeMessage(int index);

    ChatMessage *messageAt(int index) const;
    ChatMessage *messageById(const QString &id);

signals:
    void messageAdded(ChatMessage *message);
    void messageRemoved(int index);
    void messageChanged(int index, ChatMessage *message);

private:
    QList<ChatMessage *> m_messages;
};

#endif // CHATMODEL_H
