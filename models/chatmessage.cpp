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
    , m_toolCalls()
    , m_toolContent()
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
    , m_toolCalls(other.toolCalls())
    , m_toolContent(other.toolContent())
{}

void ChatMessage::mergeToolsFrom(ToolCallEntry &tool)
{
    auto updateTool = [](const ToolCallEntry &original, const ToolCallEntry &tool) -> ToolCallEntry {
        ToolCallEntry org = original;
        if (!tool.toolTypeString().isEmpty()) {
            org.setToolType(tool.toolTypeString());
        }
        if (!tool.toolCallId().isEmpty()) {
            org.setToolCallId(tool.toolCallId());
        }
        if (!tool.arguments().isEmpty()) {
            org.setArguments(tool.arguments());
        }
        if (!tool.functionName().isEmpty()) {
            org.setFunctionName(tool.functionName());
        }
        return org;
    };

    // if from stream and tools are empty, add
    if (m_toolCalls.count() == 0) {
        m_toolCalls.append(tool);
        return;
    }

    // if from stream and id is empty, updtate first tool
    if (tool.toolCallId().isEmpty()) {
        if (tool.toolIndex() > 0 && tool.toolIndex() < m_toolCalls.count()) {
            m_toolCalls[tool.toolIndex()] = updateTool(m_toolCalls[tool.toolIndex()], tool);
        } else {
            m_toolCalls[0] = updateTool(m_toolCalls[0], tool);
        }
        return;
    }

    // if from stream find tool and updtate
    for (int i = 0; i < m_toolCalls.count(); i++) {
        if (m_toolCalls[i].toolCallId() == tool.toolCallId()) {
            m_toolCalls[i] = updateTool(m_toolCalls[i], tool);
            return;
        }
    }
}

void ChatMessage::setToolCalls(const QList<ToolCallEntry> &tools)
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

void ChatMessage::setToolContent(const QByteArray &content)
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

void ChatMessage::addTools(const QList<ToolCallEntry> &tools)
{
    m_toolCalls.append(tools);
}

QJsonObject ChatMessage::toJson() const
{
    QJsonObject root;

    // Create choices array with message object
    QJsonArray choicesArray;
    QJsonObject choiceObj;
    QJsonObject messageObj;

    switch (m_role) {
        case AssistantRole: {
            messageObj["role"] = "assistant";
            break;
        }
        case UserRole: {
            messageObj["role"] = "user";
            break;
        }
        case SystemRole: {
            messageObj["role"] = "system";
            break;
        }
        case ChatRole: {
            messageObj["role"] = "user";
            break;
        }
        case ToolingRole: {
            messageObj["role"] = "tool";
            break;
        }
        case LlmRole: {
            messageObj["role"] = "llm";
            break;
        }
        default: {
            messageObj["role"] = "user";
            break;
        }
    }

    messageObj["content"] = m_content;

    QJsonArray toolsCalls;
    for (auto &toolCall : m_toolCalls) {
        QJsonObject tc;
        tc["type"] = toolCall.toolType();
        tc["id"] = toolCall.toolCallId();
        tc["index"] = toolCall.toolIndex();
        tc["function"] = toolCall.functionName();
        tc["arguments"] = toolCall.arguments();
    }

    messageObj["tool_calls"] = toolsCalls;
    messageObj["tool_content"] = QString(m_toolContent);

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

void ChatMessage::fromJson(const QJsonObject &json)
{
    // Set basic properties
    if (json.contains("content")) {
        setContent(json["content"].toString());
    }

    if (json.contains("role")) {
        QString roleStr = json["role"].toString();
        if (roleStr == "assistant") {
            setRole(AssistantRole);
        } else if (roleStr == "user") {
            setRole(ChatRole); //align right UserRole
        } else if (roleStr == "system") {
            setRole(SystemRole);
        } else if (roleStr == "tool") {
            setRole(ToolingRole);
        } else if (roleStr == "tooling") {
            setRole(ToolingRole);
        } else if (roleStr == "llm") {
            setRole(LlmRole);
        } else {
            setRole(UserRole); // Default to user role
        }
    }

    if (json.contains("created")) {
        setCreated(json["created"].toVariant().toLongLong());
    }

    if (json.contains("id")) {
        setId(json["id"].toString());
    }

    if (json.contains("model")) {
        setModel(json["model"].toString());
    }

    if (json.contains("object")) {
        setObject(json["object"].toString());
    }

    if (json.contains("system_fingerprint")) {
        setSystemFingerprint(json["system_fingerprint"].toString());
    }

    if (json.contains("finish_reason")) {
        setFinishReason(json["finish_reason"].toString());
    }

    if (json.contains("index")) {
        setChoiceIndex(json["index"].toInt());
    }

    if (json.contains("stats")) {
        setStats(json["stats"].toObject());
    }

    if (json.contains("usage")) {
        setUsage(json["usage"].toObject());
    }

    // Handle tool calls if present
    if (json.contains("tool_calls") && json["tool_calls"].isArray()) {
        QList<ToolCallEntry> tools;
        int index = 0;

        QJsonArray toolCalls = json["tool_calls"].toArray();
        for (const QJsonValue &toolValue : toolCalls) {
            if (toolValue.isObject()) {
                QJsonObject toolObj = toolValue.toObject();

                ToolCallEntry tool;
                if (toolObj.contains("type")) {
                    tool.setToolType(toolObj["type"].toString());
                }
                if (toolObj.contains("id")) {
                    tool.setToolCallId(toolObj["id"].toString());
                }
                if (toolObj.contains("index")) {
                    tool.setToolIndex(toolObj["index"].toInt(index));
                }
                if (toolObj.contains("function")) {
                    QJsonObject function = toolObj["function"].toObject();
                    if (function.contains("name")) {
                        tool.setFunctionName(function["name"].toString());
                    }
                    if (function.contains("arguments")) {
                        tool.setArguments(function["arguments"].toString());
                    }
                }
                if (tool.isValid()) {
                    tools.append(tool);
                    index++;
                }
            }
        }

        setToolCalls(tools);
    }

    // Handle tool_content if present
    if (json.contains("tool_content")) {
        setToolContent(json["tool_content"].toString().toUtf8());
    }
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
