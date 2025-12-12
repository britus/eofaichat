#include "chatmodel.h"
#include <QJsonDocument>

ChatModel::ChatModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_messages.size();
}

QVariant ChatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_messages.size())
        return QVariant();

    ChatMessage *message = m_messages[index.row()];
    switch (role) {
        case ContentRole:
            return message->content();
        case RoleRole:
            return message->role();
        case CreatedRole:
            return message->created();
        case IdRole:
            return message->id();
        case ModelRole:
            return message->model();
        case ObjectRole:
            return message->object();
        case SystemFingerprintRole:
            return message->systemFingerprint();
        default:
            return QVariant();
    }
}

bool ChatModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_messages.size())
        return false;

    ChatMessage *message = m_messages[index.row()];
    switch (role) {
        case ContentRole:
            message->setContent(value.toString());
            break;
        case RoleRole:
            message->setRole(static_cast<ChatMessage::Role>(value.toInt()));
            break;
        case CreatedRole:
            message->setCreated(value.toLongLong());
            break;
        case IdRole:
            message->setId(value.toString());
            break;
        case ModelRole:
            message->setModel(value.toString());
            break;
        case ObjectRole:
            message->setObject(value.toString());
            break;
        case SystemFingerprintRole:
            message->setSystemFingerprint(value.toString());
            break;
        default:
            return false;
    }

    emit dataChanged(index, index, {role});
    emit messageChanged(index.row(), message);
    return true;
}

Qt::ItemFlags ChatModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

QHash<int, QByteArray> ChatModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ContentRole] = "content";
    roles[RoleRole] = "role";
    roles[CreatedRole] = "created";
    roles[IdRole] = "id";
    roles[ModelRole] = "model";
    roles[ObjectRole] = "object";
    roles[SystemFingerprintRole] = "systemFingerprint";
    return roles;
}

void ChatModel::clear()
{
    if (m_messages.isEmpty())
        return;

    beginResetModel();
    qDeleteAll(m_messages);
    m_messages.clear();
    endResetModel();
}

void ChatModel::addMessage(const ChatMessage &message)
{
    ChatMessage *cm = new ChatMessage(message);

    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(cm);
    endInsertRows();

    emit messageAdded(cm);
}

void ChatModel::addMessageFromJson(const QJsonObject &json)
{
    addMessage(ChatMessage(json, this));
}

ChatMessage *ChatModel::messageById(const QString &id)
{
    foreach (ChatMessage *message, m_messages) {
        if (message->id() == id) {
            return message;
        }
    }
    return nullptr;
}

ChatMessage *ChatModel::messageAt(int index) const
{
    if (index < 0 || index >= m_messages.size())
        return nullptr;
    return m_messages[index];
}

void ChatModel::removeMessage(int index)
{
    if (index < 0 || index >= m_messages.size())
        return;

    beginRemoveRows(QModelIndex(), index, index);
    delete m_messages.takeAt(index);
    endRemoveRows();

    emit messageRemoved(index);
}
