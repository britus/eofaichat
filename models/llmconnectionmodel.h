#ifndef LLMCONNECTIONMODEL_H
#define LLMCONNECTIONMODEL_H

#include <QAbstractTableModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

// LLMConnection data structure
class LLMConnection : public QObject
{
    Q_OBJECT

public:
    enum Endpoints {
        EndpointModels = 0,
        EndpointCompletion,
        EndpointCustom,
    };
    Q_ENUM(Endpoints)

    enum AuthType {
        AuthToken = 0,
        AuthBearer,
    };
    Q_ENUM(AuthType)

    inline const QString &name() const { return m_name; }
    inline void setName(const QString &value) { m_name = value; }
    inline const QString &provider() const { return m_provider; }
    inline void setProvider(const QString &value) { m_provider = value; }
    inline const QString &apiUrl() const { return m_apiUrl; }
    inline void setApiUrl(const QString &value) { m_apiUrl = value; }
    inline const QString &apiKey() const { return m_apiKey; }
    inline void setApiKey(const QString &value) { m_apiKey = value; }
    inline bool isEnabled() const { return m_isEnabled; }
    inline void setEnabled(const bool value) { m_isEnabled = value; }
    inline bool isDefault() const { return m_isDefault; }
    inline void setDefault(const bool value) { m_isDefault = value; }
    inline AuthType authType() const { return m_authType; }
    inline void setAuthType(const AuthType value) { m_authType = value; }
    inline bool isValid() const { return !m_name.isEmpty() && !m_provider.isEmpty() && !m_apiUrl.isEmpty(); }

    // Get endpoint URI by enumeration value
    QString endpointUri(Endpoints endpoint) const
    {
        if (m_endpoints.contains(endpoint)) {
            return m_endpoints[endpoint];
        }
        return QString();
    }

    // Set endpoint URI by enumeration value
    void setEndpointUri(Endpoints endpoint, const QString &uri) { m_endpoints[endpoint] = uri; }

    // constructor
    LLMConnection(const QString &name = "", const QString &provider = "", const QString &apiUrl = "", const QString &apiKey = "", bool def = false, bool enab = false)
        : m_name(name)
        , m_provider(provider)
        , m_apiUrl(apiUrl)
        , m_apiKey(apiKey)
        , m_isDefault(def)
        , m_isEnabled(enab)
        , m_authType(AuthToken)
        , m_endpoints()
    {}

    // Copy constructor
    LLMConnection(const LLMConnection &other)
        : m_name(other.m_name)
        , m_provider(other.m_provider)
        , m_apiUrl(other.m_apiUrl)
        , m_apiKey(other.m_apiKey)
        , m_isDefault(other.m_isDefault)
        , m_isEnabled(other.m_isEnabled)
        , m_authType(other.m_authType)
        , m_endpoints(other.m_endpoints)
    {}

    // Assignment operator
    inline LLMConnection &operator=(const LLMConnection &other)
    {
        if (this != &other) {
            m_name = other.m_name;
            m_provider = other.m_provider;
            m_apiUrl = other.m_apiUrl;
            m_apiKey = other.m_apiKey;
            m_isDefault = other.m_isDefault;
            m_isEnabled = other.m_isEnabled;
            m_authType = other.m_authType;
            m_endpoints = other.m_endpoints;
        }
        return *this;
    }

    // Comparison operators
    inline bool operator==(const LLMConnection &other) const
    {
        return (m_name == other.m_name              //
                && m_provider == other.m_provider   //
                && m_apiUrl == other.m_apiUrl       //
                && m_apiKey == other.m_apiKey       //
                && m_isDefault == other.m_isDefault //
                && m_isEnabled == other.m_isEnabled //
                && m_authType == other.m_authType   //
                && m_endpoints == other.m_endpoints);
    }

    inline bool operator!=(const LLMConnection &other) const { return !(*this == other); }

    // Optional: Less than operator for sorting
    inline bool operator<(const LLMConnection &other) const
    {
        if (m_name != other.m_name)
            return m_name < other.m_name;
        if (m_provider != other.m_provider)
            return m_provider < other.m_provider;
        if (m_apiUrl != other.m_apiUrl)
            return m_apiUrl < other.m_apiUrl;
        if (m_isEnabled != other.m_isEnabled)
            return m_isEnabled < other.m_isEnabled;
        if (m_authType != other.m_authType)
            return m_authType < other.m_authType;
        return m_isDefault < other.m_isDefault;
    }

private:
    friend class LLMConnectionModel;
    QString m_name;
    QString m_provider;
    QString m_apiUrl;
    QString m_apiKey;
    bool m_isDefault;
    bool m_isEnabled;
    AuthType m_authType;
    QMap<Endpoints, QString> m_endpoints;
};
Q_DECLARE_METATYPE(LLMConnection::Endpoints)
Q_DECLARE_METATYPE(LLMConnection::AuthType)

// --------------------------------------------------

class LLMConnectionModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit LLMConnectionModel(QObject *parent = nullptr);
    ~LLMConnectionModel();

    enum Roles {
        NameRole = Qt::UserRole + 1,
        ProviderRole,
        ApiUrlRole,
        ApiKeyRole,
        AuthTypeRole,
        IsDefaultRole,
        IsEnabledRole,
        EndpointsRole,
    };
    Q_ENUM(Roles)

    enum Column {
        NameColumn = 0,
        ProviderColumn,
        ApiUrlColumn,
        IsDefaultColumn,
        IsEnabledColumn,
        ColumnCount,
    };
    Q_ENUM(Column)

    // Methods to manage connections
    void loadConnections();
    void saveConnections();
    QList<LLMConnection> getAllConnections() const;
    LLMConnection getConnection(const QString &name) const;
    void addConnection(const LLMConnection &connection);
    void updateConnection(const QString &name, const LLMConnection &connection);
    void removeConnection(const QString &name);
    void setDefaultConnection(const QString &name);
    void setDefaultConnection(const LLMConnection &connection);
    LLMConnection defaultConnection() const;

    // QAbstractTableModel methods
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

private:
    QString getConfigFileName() const;
    void loadDefaultConnections();

private:
    QMap<QString, LLMConnection> m_connections;
};
Q_DECLARE_METATYPE(LLMConnectionModel::Roles)
Q_DECLARE_METATYPE(LLMConnection)

#endif // LLMCONNECTIONMODEL_H
