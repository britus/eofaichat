#include "chatmodel.h"
#include <QJsonDocument>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

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

ChatMessage *ChatModel::addMessage(const ChatMessage &message)
{
    ChatMessage *cm = new ChatMessage(message);

    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(cm);
    endInsertRows();

    emit messageAdded(cm);
    return cm;
}

ChatMessage *ChatModel::addMessage(ChatMessage *message)
{
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(message);
    endInsertRows();

    emit messageAdded(message);
    return message;
}

QByteArray ChatModel::chatContent() const
{
    QByteArray result;
    foreach (ChatMessage *message, m_messages) {
        if (message->toolContent().isEmpty()) {
            if (!message->content().isEmpty()) {
                QString role = "user";
                switch (message->role()) {
                    case ChatMessage::UserRole: {
                        role = "user";
                        break;
                    }
                    case ChatMessage::SystemRole: {
                        role = "system";
                        break;
                    }
                    case ChatMessage::AssistantRole: {
                        role = "assistant";
                        break;
                    }
                    case ChatMessage::ChatRole: {
                        role = "user";
                        break;
                    }
                    default: {
                        role = "user";
                        break;
                    }
                }
                role = QStringLiteral("\n%1\n").arg(role);
                result.append(role.toUtf8());
                result.append(message->content().toUtf8());
                result.append("\n\n");
            }
        } else {
            result.append("\nassistant\n");
            result.append(message->toolContent().toUtf8());
            result.append("\n\n");
        }
    }
    return result;
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

// Save chat messages to a JSON file
bool ChatModel::saveToFile(const QString &fileName) const
{
    QJsonArray messagesArray;
    
    // Convert each message to JSON
    for (const ChatMessage *message : m_messages) {
        QJsonObject messageObj = message->toJson();
        messagesArray.append(messageObj);
    }
    
    // Create the root object
    QJsonObject rootObject;
    rootObject["messages"] = messagesArray;
    
    // Create JSON document
    QJsonDocument doc(rootObject);
    
    // Write to file
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << fileName;
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Compact));
    file.close();
    
    return true;
}

// Load chat messages from a JSON file
bool ChatModel::loadFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for reading:" << fileName;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc(QJsonDocument::fromJson(data));
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON in file:" << fileName;
        return false;
    }
    
    QJsonObject rootObject = doc.object();
    QJsonArray messagesArray = rootObject["messages"].toArray();
    
    // Clear current messages
    clear();
    
    // Load each message from JSON
    for (const QJsonValue &messageValue : messagesArray) {
        if (messageValue.isObject()) {
            QJsonObject messageObj = messageValue.toObject();
            
            // Create a new ChatMessage from JSON
            ChatMessage *message = new ChatMessage(this);
            message->fromJson(messageObj);
            
            // Add to model
            addMessage(message);
        }
    }
    
    return true;
}