#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>

class LLMChatClient;
class ChatMessage : public QObject
{
    Q_OBJECT

public:
    enum Role {
        None = 0,
        AssistantRole = 1,
        UserRole = 2,
        SystemRole = 3,
        ChatRole = 4,
    };
    Q_ENUM(Role)

    enum ToolType {
        Tool = 0,
        Resuource = 1,
        Prompt = 2,
        Function = 3,
    };
    Q_ENUM(ToolType)

    struct ToolEntry
    {
        QString m_toolType;
        QString m_toolCallId;
        QString m_functionName;
        //"{\"project_directory\":\"/Users/eofmc/EoF/qtmcp\"}"
        QString m_arguments;
        inline ToolType toolType() const
        {
            if (m_toolType.toLower() == "tool") {
                return Tool;
            } else if (m_toolType.toLower() == "resource") {
                return Resuource;
            } else if (m_toolType.toLower() == "prompt") {
                return Prompt;
            } else if (m_toolType.toLower() == "function") {
                return Function;
            }
            return Function;
        }
        inline void setToolType(const QString &value)
        {
            if (!value.trimmed().isEmpty())
                m_toolType = value.trimmed();
        }
        inline const QString &toolCallId() const { return m_toolCallId; }
        inline void setToolCallId(const QString &value)
        {
            if (!value.trimmed().isEmpty())
                m_toolCallId = value.trimmed();
        }
        inline const QString &functionName() const { return m_functionName; }
        inline void setFunctionName(const QString &value)
        {
            if (!value.trimmed().isEmpty())
                m_functionName = value.trimmed();
        }
        inline const QString &arguments() const { return m_arguments; }
        inline void setArguments(const QString &value)
        {
            if (!value.trimmed().isEmpty())
                m_arguments = value.trimmed();
        }
    };

    explicit ChatMessage(QObject *parent);
    ChatMessage(const QJsonObject &json, QObject *parent);
    ChatMessage(const ChatMessage &other); // Copy constructor
    QJsonObject toJson() const;

    inline const QString &content() const { return m_content; }
    inline Role role() const { return m_role; }
    inline qint64 created() const { return m_created; }
    inline const QString &id() const { return m_id; }
    inline const QString &model() const { return m_model; }
    inline const QString &object() const { return m_object; }
    inline const QString &systemFingerprint() const { return m_systemFingerprint; }
    inline const QString &finishReason() const { return m_finishReason; }
    inline int choiceIndex() const { return m_choiceIndex; }
    inline const QJsonObject &stats() const { return m_stats; }
    inline const QJsonObject &usage() const { return m_usage; }
    inline const QList<ToolEntry> &tools() const { return m_tools; }

    void mergeMessage(ChatMessage *other);
    void mergeToolsFrom(ChatMessage::ToolEntry &tool);

public slots:
    void addContent(const QString &content);
    void setContent(const QString &content);
    void setRole(ChatMessage::Role role);
    void setCreated(qint64 created);
    void setId(const QString &id);
    void setModel(const QString &model);
    void setObject(const QString &object);
    void setSystemFingerprint(const QString &systemFingerprint);
    void setFinishReason(const QString &finishReason);
    void setChoiceIndex(int choiceIndex);
    void setStats(const QJsonObject &stats);
    void setUsage(const QJsonObject &usage);
    void addTools(const QList<ChatMessage::ToolEntry> &tools);
    void setTools(const QList<ChatMessage::ToolEntry> &tools);

private:
    inline void parseHeaderFields(const QJsonObject &json);
    inline void parseMessageContent(const QJsonObject &json);
    inline void parseChoiceObject(const QJsonObject &choice);
    inline void parseToolCalls(const QJsonValue toolCalls);
    inline ToolEntry parseToolCall(const QJsonObject toolObject) const;

private:
    QString m_content;
    Role m_role;
    qint64 m_created;
    QString m_id;
    QString m_model;
    QString m_object;
    QString m_systemFingerprint;
    QString m_finishReason;
    int m_choiceIndex;
    QJsonObject m_stats;
    QJsonObject m_usage;
    QList<ToolEntry> m_tools;
    QMap<QString, QJsonObject> m_activeToolCalls; // Track ongoing tool calls by ID
};
Q_DECLARE_METATYPE(ChatMessage::ToolEntry)
Q_DECLARE_METATYPE(ChatMessage::ToolType)

// Comparison operators
bool operator==(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator!=(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator<(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator>(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator<=(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator>=(const ChatMessage &lhs, const ChatMessage &rhs);

#endif // CHATMESSAGE_H
