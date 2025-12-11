#include "chatmessage.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

ChatMessage::ChatMessage(QObject *parent)
    : QObject(parent)
    , m_role(AssistantRole)
    , m_created(0)
    , m_choiceIndex(0)
{}

ChatMessage::ChatMessage(const QJsonObject &json, QObject *parent)
    : QObject(parent)
{
    // Parse the JSON structure to extract message content and role
    parseMessageContent(json);

    // Parse other fields from the JSON structure
    parseOtherFields(json);
}

// Copy constructor
ChatMessage::ChatMessage(const ChatMessage &other)
    : QObject(other.parent())
    , m_content(other.content())
    , m_role(other.role())
    , m_created(other.created())
    , m_id(other.id())
    , m_model(other.model())
    , m_object(other.object())
    , m_systemFingerprint(other.systemFingerprint())
    , m_finishReason(other.finishReason())
    , m_choiceIndex(other.choiceIndex())
    , m_stats(other.stats())
    , m_usage(other.usage())
{}

void ChatMessage::parseToolCalls(const QJsonValue toolCalls)
{
    QJsonArray toolCallsArray = toolCalls.toArray();
    for (int i = 0; i < toolCallsArray.count(); i++) {
        if (!toolCallsArray[i].isObject()) {
            continue;
        }
        ToolEntry tool = {};
        QJsonObject toolObject = toolCallsArray[i].toObject();
        // Extract type
        QJsonValue typeValue = toolObject["type"];
        if (typeValue.isString()) {
            tool.setToolType(typeValue.toString());
        }
        // Extract id
        QJsonValue idValue = toolObject["id"];
        if (idValue.isString()) {
            tool.setToolCallId(idValue.toString());
        }
        // Extract function information
        QJsonValue functionValue = toolObject["function"];
        if (functionValue.isObject()) {
            QJsonObject functionObj = functionValue.toObject();
            // Extract name
            QJsonValue nameValue = functionObj["name"];
            if (nameValue.isString()) {
                tool.setFunctionName(nameValue.toString());
            }
            // Extract arguments kv array
            QJsonValue argumentsValue = functionObj["arguments"];
            if (argumentsValue.isString()) {
                tool.setArguments(argumentsValue.toString());
            }
        }
        m_tools.append(tool);
    }

    foreach (auto t, m_tools) {
        qDebug("[ChatMessage] Tool to call: %s::%s %s", //
               qPrintable(t.m_toolType),
               qPrintable(t.functionName()),
               qPrintable(t.arguments()));
    }

    emit toolsChanged();
}

void ChatMessage::parseMessageContent(const QJsonObject &json)
{
    // Initialize default values
    m_finishReason = QString();
    m_choiceIndex = 0;
    m_stats = QJsonObject();
    m_usage = QJsonObject();
    m_content = QString();
    m_role = AssistantRole;

    // Extract message content and role with error handling
    QJsonValue choicesValue = json["choices"];
    if (choicesValue.isArray()) {
        QJsonArray choicesArray = choicesValue.toArray();
        if (!choicesArray.isEmpty()) {
            QJsonValue choice = choicesArray.first();
            if (choice.isObject()) {
                QJsonObject choiceObj = choice.toObject();

                // Extract finish_reason
                QJsonValue finishReasonValue = choiceObj["finish_reason"];
                if (finishReasonValue.isString()) {
                    m_finishReason = finishReasonValue.toString();
                }

                // Extract index
                QJsonValue indexValue = choiceObj["index"];
                if (indexValue.isDouble()) {
                    m_choiceIndex = static_cast<int>(indexValue.toDouble());
                }

                // Extract stats
                QJsonValue statsValue = json["stats"];
                if (statsValue.isObject()) {
                    m_stats = statsValue.toObject();
                }

                // Extract usage
                QJsonValue usageValue = json["usage"];
                if (usageValue.isObject()) {
                    m_usage = usageValue.toObject();
                }

                // normale message or tool call item
                QJsonValue messageValue = choiceObj["message"];
                if (messageValue.isObject()) {
                    QJsonObject messageObj = messageValue.toObject();

                    // Extract content with error handling
                    QJsonValue contentValue = messageObj["content"];
                    if (contentValue.isString()) {
                        m_content = contentValue.toString();
                    } else {
                        m_content = QString(); // Empty string as default
                    }

                    // Extract tool_calls
                    QJsonValue toolCallsValue = messageObj["tool_calls"];
                    if (toolCallsValue.isArray() && !toolCallsValue.toArray().isEmpty()) {
                        parseToolCalls(toolCallsValue);
                    }

                    // Extract role with error handling
                    QJsonValue roleValue = messageObj["role"];
                    if (roleValue.isString()) {
                        QString roleStr = roleValue.toString();
                        if (roleStr == "assistant") {
                            m_role = AssistantRole;
                        } else if (roleStr == "user") {
                            m_role = UserRole;
                        } else if (roleStr == "system") {
                            m_role = SystemRole;
                        } else {
                            m_role = AssistantRole; // Default
                        }
                    }
                }
            }
        }
    }
}

void ChatMessage::parseOtherFields(const QJsonObject &json)
{
    // Extract other fields with error handling
    QJsonValue createdValue = json["created"];
    if (createdValue.isDouble()) {
        m_created = static_cast<qint64>(createdValue.toDouble());
    } else if (createdValue.isString()) {
        bool ok;
        qint64 value = createdValue.toString().toLongLong(&ok);
        if (ok) {
            m_created = value;
        } else {
            m_created = 0; // Default value
        }
    } else {
        m_created = 0; // Default value
    }

    QJsonValue idValue = json["id"];
    if (idValue.isString()) {
        m_id = idValue.toString();
    } else {
        m_id = QString(); // Empty string as default
    }

    QJsonValue modelValue = json["model"];
    if (modelValue.isString()) {
        m_model = modelValue.toString();
    } else {
        m_model = QString(); // Empty string as default
    }

    QJsonValue objectValue = json["object"];
    if (objectValue.isString()) {
        m_object = objectValue.toString();
    } else {
        m_object = QString(); // Empty string as default
    }

    QJsonValue systemFingerprintValue = json["system_fingerprint"];
    if (systemFingerprintValue.isString()) {
        m_systemFingerprint = systemFingerprintValue.toString();
    } else {
        m_systemFingerprint = QString(); // Empty string as default
    }
}

QString ChatMessage::content() const
{
    return m_content;
}

ChatMessage::Role ChatMessage::role() const
{
    return m_role;
}

qint64 ChatMessage::created() const
{
    return m_created;
}

QString ChatMessage::id() const
{
    return m_id;
}

QString ChatMessage::model() const
{
    return m_model;
}

QString ChatMessage::object() const
{
    return m_object;
}

QString ChatMessage::systemFingerprint() const
{
    return m_systemFingerprint;
}

QString ChatMessage::finishReason() const
{
    return m_finishReason;
}

int ChatMessage::choiceIndex() const
{
    return m_choiceIndex;
}

QJsonObject ChatMessage::stats() const
{
    return m_stats;
}

QJsonObject ChatMessage::usage() const
{
    return m_usage;
}

void ChatMessage::setContent(const QString &content)
{
    if (m_content != content) {
        m_content = content;
        emit contentChanged();
    }
}

void ChatMessage::setRole(Role role)
{
    if (m_role != role) {
        m_role = role;
        emit roleChanged();
    }
}

void ChatMessage::setCreated(qint64 created)
{
    if (m_created != created) {
        m_created = created;
        emit createdChanged();
    }
}

void ChatMessage::setId(const QString &id)
{
    if (m_id != id) {
        m_id = id;
        emit idChanged();
    }
}

void ChatMessage::setModel(const QString &model)
{
    if (m_model != model) {
        m_model = model;
        emit modelChanged();
    }
}

void ChatMessage::setObject(const QString &object)
{
    if (m_object != object) {
        m_object = object;
        emit objectChanged();
    }
}

void ChatMessage::setSystemFingerprint(const QString &systemFingerprint)
{
    if (m_systemFingerprint != systemFingerprint) {
        m_systemFingerprint = systemFingerprint;
        emit systemFingerprintChanged();
    }
}

void ChatMessage::setFinishReason(const QString &finishReason)
{
    if (m_finishReason != finishReason) {
        m_finishReason = finishReason;
        emit finishReasonChanged();
    }
}

void ChatMessage::setChoiceIndex(int choiceIndex)
{
    if (m_choiceIndex != choiceIndex) {
        m_choiceIndex = choiceIndex;
        emit choiceIndexChanged();
    }
}

void ChatMessage::setStats(const QJsonObject &stats)
{
    if (m_stats != stats) {
        m_stats = stats;
        emit statsChanged();
    }
}

void ChatMessage::setUsage(const QJsonObject &usage)
{
    if (m_usage != usage) {
        m_usage = usage;
        emit usageChanged();
    }
}

const QList<ChatMessage::ToolEntry> &ChatMessage::tools() const
{
    return m_tools;
}

QJsonObject ChatMessage::toJson() const
{
    QJsonObject root;

    // Create choices array with message object
    QJsonArray choicesArray;
    QJsonObject choiceObj;
    QJsonObject messageObj;

    messageObj["content"] = m_content;
    messageObj["role"] = (m_role == AssistantRole) ? "assistant" : (m_role == UserRole) ? "user" : "system";
    messageObj["tool_calls"] = QJsonArray();

    choiceObj["finish_reason"] = m_finishReason;
    choiceObj["index"] = m_choiceIndex;
    choiceObj["logprobs"] = QJsonValue::Null;
    choiceObj["message"] = messageObj;

    choicesArray.append(choiceObj);
    root["choices"] = choicesArray;

    // Add other fields
    root["created"] = m_created;
    root["id"] = m_id;
    root["model"] = m_model;
    root["object"] = m_object;
    root["system_fingerprint"] = m_systemFingerprint;

    // Add stats and usage objects
    root["stats"] = m_stats;
    root["usage"] = m_usage;

    return root;
}

// Comparison operators
bool operator==(const ChatMessage &lhs, const ChatMessage &rhs)
{
    return lhs.created() == rhs.created() && lhs.id() == rhs.id() && lhs.model() == rhs.model() && lhs.systemFingerprint() == rhs.systemFingerprint();
}

bool operator!=(const ChatMessage &lhs, const ChatMessage &rhs)
{
    return !(lhs == rhs);
}

bool operator<(const ChatMessage &lhs, const ChatMessage &rhs)
{
    if (lhs.created() != rhs.created()) {
        return lhs.created() < rhs.created();
    }
    if (lhs.id() != rhs.id()) {
        return lhs.id() < rhs.id();
    }
    if (lhs.model() != rhs.model()) {
        return lhs.model() < rhs.model();
    }
    return lhs.systemFingerprint() < rhs.systemFingerprint();
}

bool operator>(const ChatMessage &lhs, const ChatMessage &rhs)
{
    return rhs < lhs;
}

bool operator<=(const ChatMessage &lhs, const ChatMessage &rhs)
{
    return !(lhs > rhs);
}

bool operator>=(const ChatMessage &lhs, const ChatMessage &rhs)
{
    return !(lhs < rhs);
}
