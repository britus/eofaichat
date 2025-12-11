#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>

class ChatMessage : public QObject
{
    Q_OBJECT

public:
    enum Role {
        AssistantRole = 0,
        UserRole = 1,
        SystemRole = 2,
        ChatRole = 3,
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
        inline void setToolType(const QString &value) { m_toolType = value; }
        inline const QString &toolCallId() const { return m_toolCallId; }
        inline void setToolCallId(const QString &value) { m_toolCallId = value; }
        inline const QString &functionName() const { return m_functionName; }
        inline void setFunctionName(const QString &value) { m_functionName = value; }
        inline const QString &arguments() const { return m_arguments; }
        inline void setArguments(const QString &value) { m_arguments = value; }
    };

    explicit ChatMessage(QObject *parent = nullptr);
    ChatMessage(const QJsonObject &json, QObject *parent = nullptr);
    ChatMessage(const ChatMessage &other); // Copy constructor

    QString content() const;
    Role role() const;
    qint64 created() const;
    QString id() const;
    QString model() const;
    QString object() const;
    QString systemFingerprint() const;
    // Additional fields for tool calls
    QString finishReason() const;
    int choiceIndex() const;
    QJsonObject stats() const;
    QJsonObject usage() const;
    const QList<ToolEntry> &tools() const;
    QJsonObject toJson() const;

public slots:
    void setContent(const QString &content);
    void setRole(ChatMessage::Role role);
    void setCreated(qint64 created);
    void setId(const QString &id);
    void setModel(const QString &model);
    void setObject(const QString &object);
    void setSystemFingerprint(const QString &systemFingerprint);
    // Additional setters for tool calls
    void setFinishReason(const QString &finishReason);
    void setChoiceIndex(int choiceIndex);
    void setStats(const QJsonObject &stats);
    void setUsage(const QJsonObject &usage);

signals:
    void contentChanged();
    void roleChanged();
    void createdChanged();
    void idChanged();
    void modelChanged();
    void objectChanged();
    void systemFingerprintChanged();
    // Additional signals for tool calls
    void finishReasonChanged();
    void choiceIndexChanged();
    void statsChanged();
    void usageChanged();
    void toolsChanged();

private:
    void parseToolCalls(const QJsonValue toolCalls);
    void parseMessageContent(const QJsonObject &json);
    void parseOtherFields(const QJsonObject &json);

private:
    Q_PROPERTY(QString content READ content WRITE setContent NOTIFY contentChanged FINAL)
    Q_PROPERTY(Role role READ role WRITE setRole NOTIFY roleChanged FINAL)
    Q_PROPERTY(qint64 created READ created WRITE setCreated NOTIFY createdChanged FINAL)
    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged FINAL)
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(QString object READ object WRITE setObject NOTIFY objectChanged FINAL)
    Q_PROPERTY(QString systemFingerprint READ systemFingerprint WRITE setSystemFingerprint NOTIFY systemFingerprintChanged FINAL)
    Q_PROPERTY(QString finishReason READ finishReason WRITE setFinishReason NOTIFY finishReasonChanged FINAL)
    Q_PROPERTY(int choiceIndex READ choiceIndex WRITE setChoiceIndex NOTIFY choiceIndexChanged FINAL)
    Q_PROPERTY(QJsonObject stats READ stats WRITE setStats NOTIFY statsChanged FINAL)
    Q_PROPERTY(QJsonObject usage READ usage WRITE setUsage NOTIFY usageChanged FINAL)

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

    // Additional fields for tool calls
    QList<ToolEntry> m_tools;
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
