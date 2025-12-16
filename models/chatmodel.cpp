#include "chatmodel.h"
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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

inline void ChatModel::reportError(const QString &message)
{
    qCritical("[LLMChatClient] ERROR: %s", qPrintable(message));

    emit errorOccurred("Server URL not set");
}

void ChatModel::onParseDataStream(const QByteArray &data)
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

        onParseMessageObject(doc.object());
    }
}

void ChatModel::onParseMessageObject(const QJsonObject &response)
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
    message = messageById(value.toString());
    if (message == nullptr) {
        message = new ChatMessage(this);
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
        addMessage(message);
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

inline bool ChatModel::validateValue(const QJsonValue &value, const QString &key, const QJsonValue::Type expectedType)
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

inline bool ChatModel::valueOf(const QJsonObject &response, const QString &key, const QJsonValue::Type expectedType, QJsonValue &value)
{
    value = response.value(key);
    return validateValue(value, key, expectedType);
}

inline bool ChatModel::parseToolCall(const QJsonObject toolObject, ChatMessage::ToolEntry &tool) const
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

inline bool ChatModel::parseToolCalls(ChatMessage *message, const QJsonArray &toolCalls)
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

inline bool ChatModel::parseChoiceObject(ChatMessage *message, const QJsonObject &choiceObject)
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

inline bool ChatModel::parseChoices(ChatMessage *message, const QJsonArray &choices)
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

inline void ChatModel::checkAndRunTooling(ChatMessage *message)
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
