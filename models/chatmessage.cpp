#include <chatmessage.h>
#include <llmchatclient.h>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

ChatMessage::ChatMessage(QObject *parent)
    : QObject(parent)
    , m_content()
    , m_role(UserRole)
    , m_created(0)
    , m_id()
    , m_model()
    , m_object()
    , m_systemFingerprint()
    , m_finishReason()
    , m_choiceIndex(0)
    , m_stats()
    , m_usage()
    , m_tools()
{}

ChatMessage::ChatMessage(const QJsonObject &json, QObject *parent)
    : QObject(parent)
    , m_content()
    , m_role(AssistantRole)
    , m_created(0)
    , m_id()
    , m_model()
    , m_object()
    , m_systemFingerprint()
    , m_finishReason()
    , m_choiceIndex(0)
    , m_stats()
    , m_usage()
    , m_tools()
{
    // Parse other fields from the JSON structure
    parseHeaderFields(json);
    // Parse the JSON structure to extract message content and role
    parseMessageContent(json);
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
    , m_tools(other.tools())
{}

inline void ChatMessage::parseHeaderFields(const QJsonObject &json)
{
    // Extract other fields with error handling
    QJsonValue createdValue = json["created"];
    if (!createdValue.isNull() && createdValue.isDouble()) {
        m_created = static_cast<qint64>(createdValue.toDouble());
    } else if (!createdValue.isNull() && createdValue.isString()) {
        bool ok;
        qint64 value = createdValue.toString().toLongLong(&ok);
        if (ok) {
            m_created = value;
        }
    }
    QJsonValue idValue = json["id"];
    if (!idValue.isNull() && idValue.isString()) {
        m_id = idValue.toString();
    }

    QJsonValue modelValue = json["model"];
    if (!modelValue.isNull() && modelValue.isString()) {
        m_model = modelValue.toString();
    }

    QJsonValue objectValue = json["object"];
    if (!objectValue.isNull() && objectValue.isString()) {
        m_object = objectValue.toString();
    }

    QJsonValue systemFingerprintValue = json["system_fingerprint"];
    if (!systemFingerprintValue.isNull() && systemFingerprintValue.isString()) {
        m_systemFingerprint = systemFingerprintValue.toString();
    }
}

inline void ChatMessage::parseMessageContent(const QJsonObject &message)
{
    //qDebug() << "[ChatMessage] parse LLM message object:" << message;

    // Extract stats
    QJsonValue statsValue = message["stats"];
    if (!statsValue.isNull() && statsValue.isObject()) {
        m_stats = statsValue.toObject();
    }

    // Extract usage
    QJsonValue usageValue = message["usage"];
    if (!usageValue.isNull() && usageValue.isObject()) {
        m_usage = usageValue.toObject();
    }

    // Extract message content
    QJsonValue choicesValue = message["choices"];
    if (!choicesValue.isNull() && choicesValue.isArray()) {
        QJsonArray choicesArray = choicesValue.toArray();
        if (!choicesArray.isEmpty()) {
            for (int i = 0; i < choicesArray.count(); i++) {
                QJsonValue choice = choicesArray.at(i);
                if (!choice.isObject()) {
                    continue;
                }
                parseChoiceObject(choice.toObject());
            }
            return;
        }
    }

    // Try stream line object
    parseChoiceObject(message);
}

inline void ChatMessage::parseChoiceObject(const QJsonObject &choice)
{
    //qDebug() << "[ChatMessage] parse choice object:" << choice;

    // Extract finish_reason
    QJsonValue finishReasonValue = choice["finish_reason"];
    if (!finishReasonValue.isNull() && finishReasonValue.isString()) {
        m_finishReason = finishReasonValue.toString();
    }

    // Extract index
    QJsonValue indexValue = choice["index"];
    if (!indexValue.isNull() && indexValue.isDouble()) {
        m_choiceIndex = static_cast<int>(indexValue.toDouble());
    }

    // Extract stream delta object
    QJsonObject message;
    QJsonValue delta = choice["delta"];
    if (!delta.isNull() && delta.isObject()) {
        // Set message object from delta
        message["message"] = delta.toObject();
        parseChoiceObject(message);
        return;
    }

    // normale message or tool call item
    QJsonValue messageValue = choice["message"];
    if (!messageValue.isNull() && messageValue.isObject()) {
        QJsonObject messageObj = messageValue.toObject();

        // Extract content with error handling
        QJsonValue contentValue = messageObj["content"];
        if (!contentValue.isNull() && contentValue.isString()) {
            m_content = contentValue.toString();
        }

        // Extract message role
        QJsonValue roleValue = messageObj["role"];
        if (!roleValue.isNull() && roleValue.isString()) {
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

        // Extract tool_calls
        QJsonValue toolCallsValue = messageObj["tool_calls"];
        if (!toolCallsValue.isNull() && toolCallsValue.isArray()) {
            QJsonArray tools = toolCallsValue.toArray();
            if (!tools.isEmpty()) {
                parseToolCalls(tools);
            }
        }
    }
}

inline void ChatMessage::parseToolCalls(const QJsonValue toolCalls)
{
    QJsonArray toolCallsArray = toolCalls.toArray();
    for (int i = 0; i < toolCallsArray.count(); i++) {
        if (!toolCallsArray[i].isObject()) {
            qWarning("[ChatMessage] parseToolCalls: Object is not an array, skip.");
            continue;
        }
        QJsonObject toolObject = toolCallsArray[i].toObject();
        m_tools.append(parseToolCall(toolObject));
    }
}

inline ChatMessage::ToolEntry ChatMessage::parseToolCall(const QJsonObject toolObject) const
{
    ToolEntry tool = {};

    qDebug() << "[ChatMessage] parseToolCall with:" << toolObject;

    // Extract type
    QJsonValue typeValue = toolObject["type"];
    if (!typeValue.isNull() && typeValue.isString()) {
        tool.setToolType(typeValue.toString());
    }

    // Extract id
    QJsonValue idValue = toolObject["id"];
    if (!idValue.isNull() && idValue.isString()) {
        tool.setToolCallId(idValue.toString());
    }

    // Extract function information
    QJsonValue functionValue = toolObject["function"];
    if (!functionValue.isNull() && functionValue.isObject()) {
        QJsonObject functionObj = functionValue.toObject();
        // Extract name
        QJsonValue nameValue = functionObj["name"];
        if (!nameValue.isNull() && nameValue.isString()) {
            tool.setFunctionName(nameValue.toString());
        }
        // Extract arguments (should be a JSON object)
        QJsonValue argumentsValue = functionObj["arguments"];
        if (!argumentsValue.isNull() && argumentsValue.isString()) {
            tool.setArguments(argumentsValue.toString());
        }
    }

    return tool;
}

void ChatMessage::mergeToolsFrom(ChatMessage::ToolEntry &tool)
{
    auto updateTool = [](const ToolEntry &original, const ToolEntry &tool) -> ToolEntry const {
        ToolEntry t = original;
        if (!tool.m_toolType.isEmpty()) {
            t.setToolType(tool.m_toolType);
        }
        if (!tool.m_toolCallId.isEmpty()) {
            t.setToolCallId(tool.m_toolCallId);
        }
        if (!tool.m_arguments.isEmpty()) {
            t.setArguments(tool.m_arguments);
        }
        if (!tool.m_functionName.isEmpty()) {
            t.setFunctionName(tool.m_functionName);
        }
        return t;
    };

    // if from stream and tools are empty, add
    if (m_tools.count() == 0) {
        m_tools.append(tool);
        return;
    }

    // if from stream and id is empty, updtate first tool
    if (tool.toolCallId().isEmpty()) {
        m_tools[0] = updateTool(m_tools[0], tool);
        return;
    }

    // if from stream find tool and updtate
    for (int i = 0; i < m_tools.count(); i++) {
        if (m_tools[i].toolCallId() == tool.toolCallId()) {
            m_tools[i] = updateTool(m_tools[i], tool);
            return;
        }
    }
}

void ChatMessage::mergeMessage(ChatMessage *other)
{
    if (id() != other->id()) {
        return;
    }
    setContent(other->content());
    setRole(other->role());
    setCreated(other->created());
    setModel(other->model());
    setObject(other->object());
    setSystemFingerprint(other->systemFingerprint());
    setFinishReason(other->finishReason());
    setChoiceIndex(other->choiceIndex());
    setStats(other->stats());
    setUsage(other->usage());
    setTools(other->tools());
}

void ChatMessage::setTools(const QList<ToolEntry> &tools)
{
    if (!tools.isEmpty()) {
        foreach (auto tool, tools) {
            mergeToolsFrom(tool);
        }
    }
}

void ChatMessage::setContent(const QString &content)
{
    if (!content.isEmpty() && m_content != content) {
        m_content = content;
    }
}

void ChatMessage::addContent(const QString &content)
{
    if (!content.isEmpty() && !content.isEmpty()) {
        m_content += content;
    }
}

void ChatMessage::setRole(Role role)
{
    if (role != None && m_role != role) {
        m_role = role;
    }
}

void ChatMessage::setCreated(qint64 created)
{
    if (m_created != created) {
        m_created = created;
    }
}

void ChatMessage::setId(const QString &id)
{
    if (!id.isEmpty() && m_id != id) {
        m_id = id;
    }
}

void ChatMessage::setModel(const QString &model)
{
    if (!model.isEmpty() && m_model != model) {
        m_model = model;
    }
}

void ChatMessage::setObject(const QString &object)
{
    if (!object.isEmpty() && m_object != object) {
        m_object = object;
    }
}

void ChatMessage::setSystemFingerprint(const QString &systemFingerprint)
{
    if (!systemFingerprint.isEmpty() && m_systemFingerprint != systemFingerprint) {
        m_systemFingerprint = systemFingerprint;
    }
}

void ChatMessage::setFinishReason(const QString &finishReason)
{
    if (!finishReason.isEmpty() && m_finishReason != finishReason) {
        m_finishReason = finishReason;
    }
}

void ChatMessage::setChoiceIndex(int choiceIndex)
{
    if (m_choiceIndex != choiceIndex) {
        m_choiceIndex = choiceIndex;
    }
}

void ChatMessage::setStats(const QJsonObject &stats)
{
    if (!stats.isEmpty() && m_stats != stats) {
        m_stats = stats;
    }
}

void ChatMessage::setUsage(const QJsonObject &usage)
{
    if (!usage.isEmpty() && m_usage != usage) {
        m_usage = usage;
    }
}

void ChatMessage::addTools(const QList<ChatMessage::ToolEntry> &tools)
{
    m_tools.append(tools);
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
