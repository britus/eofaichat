#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>

class ChatMessage : public QObject
{
    Q_OBJECT

public:
    enum Role { AssistantRole = 0, UserRole = 1, SystemRole = 2, ChatRole = 3 };
    Q_ENUM(Role)

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
    QJsonObject toJson() const;

public slots:
    void setContent(const QString &content);
    void setRole(ChatMessage::Role role);
    void setCreated(qint64 created);
    void setId(const QString &id);
    void setModel(const QString &model);
    void setObject(const QString &object);
    void setSystemFingerprint(const QString &systemFingerprint);

signals:
    void contentChanged();
    void roleChanged();
    void createdChanged();
    void idChanged();
    void modelChanged();
    void objectChanged();
    void systemFingerprintChanged();

private:
    Q_PROPERTY(QString content READ content WRITE setContent NOTIFY contentChanged FINAL)
    Q_PROPERTY(Role role READ role WRITE setRole NOTIFY roleChanged FINAL)
    Q_PROPERTY(qint64 created READ created WRITE setCreated NOTIFY createdChanged FINAL)
    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged FINAL)
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY modelChanged FINAL)
    Q_PROPERTY(QString object READ object WRITE setObject NOTIFY objectChanged FINAL)
    Q_PROPERTY(QString systemFingerprint READ systemFingerprint WRITE setSystemFingerprint NOTIFY systemFingerprintChanged FINAL)

    QString m_content;
    Role m_role;
    qint64 m_created;
    QString m_id;
    QString m_model;
    QString m_object;
    QString m_systemFingerprint;
};

// Comparison operators
bool operator==(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator!=(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator<(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator>(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator<=(const ChatMessage &lhs, const ChatMessage &rhs);
bool operator>=(const ChatMessage &lhs, const ChatMessage &rhs);

#endif // CHATMESSAGE_H
