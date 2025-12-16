#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QDataStream>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>

class ToolCallEntry
{
public:
    enum ToolType {
        TypeNone = 0,
        Function = 1,
        Resuource = 2, // Fixed typo: was "Resuource"
        Prompt = 3,
    };

    explicit ToolCallEntry()
        : m_toolIndex(0)
    {}

    ToolCallEntry(const ToolCallEntry &other)
        : m_toolIndex(other.m_toolIndex)
        , m_toolType(other.m_toolType)
        , m_toolCallId(other.m_toolCallId)
        , m_functionName(other.m_functionName)
        , m_arguments(other.m_arguments)
    {}

    // Assignment operator
    ToolCallEntry &operator=(const ToolCallEntry &other)
    {
        if (this != &other) { // Self-assignment check
            m_toolIndex = other.m_toolIndex;
            m_toolType = other.m_toolType;
            m_toolCallId = other.m_toolCallId;
            m_functionName = other.m_functionName;
            m_arguments = other.m_arguments;
        }
        return *this;
    }

    // Comparison operator
    bool operator==(const ToolCallEntry &other) const
    {
        return (m_toolIndex == other.m_toolIndex          //
                && m_toolType == other.m_toolType         //
                && m_toolCallId == other.m_toolCallId     //
                && m_functionName == other.m_functionName //
                && m_arguments == other.m_arguments);
    }

    inline ToolType toolType() const
    {
        if (m_toolType.toLower() == "resource") {
            return Resuource;
        } else if (m_toolType.toLower() == "prompt") {
            return Prompt;
        } else if (m_toolType.toLower() == "function") {
            return Function;
        }
        return TypeNone;
    }

    inline void setToolIndex(int value)
    {
        if (value != 0)
            m_toolIndex = value;
    }

    inline int toolIndex() const { return m_toolIndex; }

    inline void setToolType(const QString &value)
    {
        if (!value.trimmed().isEmpty())
            m_toolType = value.trimmed();
    }

    inline const QString &toolTypeString() const { return m_toolType; }

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

    inline bool isValid() const
    {
        return !m_toolType.isEmpty()      //
               && !m_toolCallId.isEmpty() //
               && !m_functionName.isEmpty();
    }

private:
    int m_toolIndex;
    QString m_toolType;
    QString m_toolCallId;
    QString m_functionName;
    QString m_arguments;
};
Q_DECLARE_METATYPE(ToolCallEntry::ToolType)

class LLMChatClient;
class ChatMessage : public QObject
{
    Q_OBJECT

public:
    enum Role {
        NoRole = 0,
        AssistantRole = 1,
        UserRole = 2,
        SystemRole = 3,
        ChatRole = 4,
        ToolingRole = 5,
        LlmRole = 6,
    };
    Q_ENUM(Role)

    explicit ChatMessage(QObject *parent);
    ChatMessage(const ChatMessage &other); // Copy constructor

    /**
     * @brief toJson
     * @return
     */
    QJsonObject toJson() const;

    /**
     * @brief fromJson
     * @param json
     */
    void fromJson(const QJsonObject &json);

    /**
     * @brief mergeToolsFrom
     * @param tool
     */
    void mergeToolsFrom(ToolCallEntry &tool);

    inline const QString &content() const { return m_content; }
    inline Role role() const { return m_role; }
    inline bool isUser() const { return m_role == ChatRole || m_role == UserRole; }
    inline qint64 created() const { return m_created; }
    inline const QString &id() const { return m_id; }
    inline const QString &model() const { return m_model; }
    inline const QString &object() const { return m_object; }
    inline const QString &systemFingerprint() const { return m_systemFingerprint; }
    inline const QString &finishReason() const { return m_finishReason; }
    inline int choiceIndex() const { return m_choiceIndex; }
    inline const QJsonObject &stats() const { return m_stats; }
    inline const QJsonObject &usage() const { return m_usage; }
    inline const QList<ToolCallEntry> &toolCalls() const { return m_toolCalls; }
    inline const QByteArray &toolContent() const { return m_toolContent; }

public slots:
    void appendContent(const QString &content);
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
    void addTools(const QList<ToolCallEntry> &tools);
    void setToolCalls(const QList<ToolCallEntry> &tools);
    void setToolContent(const QByteArray &content);

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
    QList<ToolCallEntry> m_toolCalls;
    QByteArray m_toolContent;
};

// Comparison operators
bool operator==(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator!=(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator<(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator>(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator<=(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator>=(const ChatMessage &lhs, const ChatMessage &rhs);

#endif // CHATMESSAGE_H
