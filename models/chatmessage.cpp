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

void ChatMessage::mergeToolsFrom(ChatMessage::ToolEntry &tool)
{
    auto updateTool = [](const ToolEntry &original, const ToolEntry &tool) -> ToolEntry const {
        ToolEntry org = original;
        if (!tool.m_toolType.isEmpty()) {
            org.setToolType(tool.m_toolType);
        }
        if (!tool.m_toolCallId.isEmpty()) {
            org.setToolCallId(tool.m_toolCallId);
        }
        if (!tool.m_arguments.isEmpty()) {
            org.setArguments(tool.m_arguments);
        }
        if (!tool.m_functionName.isEmpty()) {
            org.setFunctionName(tool.m_functionName);
        }
        return org;
    };

    // if from stream and tools are empty, add
    if (m_tools.count() == 0) {
        m_tools.append(tool);
        return;
    }

    // if from stream and id is empty, updtate first tool
    if (tool.toolCallId().isEmpty()) {
        if (tool.toolIndex() > 0 && tool.toolIndex() < m_tools.count()) {
            m_tools[tool.toolIndex()] = updateTool(m_tools[tool.toolIndex()], tool);
        } else {
            m_tools[0] = updateTool(m_tools[0], tool);
        }
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

void ChatMessage::setToolContent(const QString &content)
{
    if (!content.isEmpty() && m_toolContent != content) {
        m_toolContent = content;
    }
}

void ChatMessage::appendContent(const QString &content)
{
    if (!content.isEmpty() && !content.isEmpty()) {
        m_content.append(content);
    }
}

void ChatMessage::setRole(Role role)
{
    if (role != NoRole && m_role != role) {
        m_role = role;
    }
}

void ChatMessage::setCreated(qint64 created)
{
    if (created != 0 && m_created != created) {
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
    if (choiceIndex != 0 && m_choiceIndex != choiceIndex) {
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
