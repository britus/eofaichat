#include "chatmessage.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

ChatMessage::ChatMessage(QObject *parent)
    : QObject(parent)
    , m_role(AssistantRole)
    , m_created(0)
{}

ChatMessage::ChatMessage(const QJsonObject &json, QObject *parent)
    : QObject(parent)
{
    /* ======================================================
    {
        "choices": [
            {
                "finish_reason": "stop",
                "index": 0,
                "logprobs": null,
                "message": {
                    "content": "Hello! It's nice to meet you! How can I help you today?",
                    "role": "assistant",
                    "tool_calls": [
                    ]
                }
            }
        ],
        "created": 1765081803,
        "id": "chatcmpl-qovkqceldxfxc4gmwvel0p",
        "model": "qwen/qwen3-coder-30b",
        "object": "chat.completion",
        "stats": {
        },
        "system_fingerprint": "qwen/qwen3-coder-30b",
        "usage": {
            "completion_tokens": 17,
            "prompt_tokens": 10,
            "total_tokens": 27
        }
    }
    ====================================================== */
    // Extract message content and role with error handling
    QJsonValue choicesValue = json["choices"];
    if (choicesValue.isArray()) {
        QJsonArray choicesArray = choicesValue.toArray();
        if (!choicesArray.isEmpty()) {
            QJsonValue choice = choicesArray.first();
            if (choice.isObject()) {
                QJsonObject choiceObj = choice.toObject();
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
                    } else {
                        m_role = AssistantRole; // Default if role is missing or not a string
                    }
                } else {
                    // Handle case where message is not an object
                    m_content = QString();
                    m_role = AssistantRole;
                }
            } else {
                // Handle case where choice is not an object
                m_content = QString();
                m_role = AssistantRole;
            }
        } else {
            // Handle empty choices array
            m_content = QString();
            m_role = AssistantRole;
        }
    } else {
        // Handle case where choices is not an array
        m_content = QString();
        m_role = AssistantRole;
    }

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
{}

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

    choiceObj["finish_reason"] = "stop";
    choiceObj["index"] = 0;
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

    // Add empty stats and usage objects
    root["stats"] = QJsonObject();
    QJsonObject usageObj;
    usageObj["completion_tokens"] = 0;
    usageObj["prompt_tokens"] = 0;
    usageObj["total_tokens"] = 0;
    root["usage"] = usageObj;

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
