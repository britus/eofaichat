#include "llmconnectionmodel.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

LLMConnectionModel::LLMConnectionModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    loadConnections();
}

LLMConnectionModel::~LLMConnectionModel() {}

void LLMConnectionModel::loadConnections()
{
    m_connections.clear();

    QString fileName = getConfigFileName();
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        // Load connections
        if (doc.isObject()) {
            QJsonObject rootObj = doc.object();
            if (rootObj.contains("connections") && rootObj["connections"].isArray()) {
                QJsonArray connectionsArray = rootObj["connections"].toArray();

                for (const auto &connectionValue : connectionsArray) {
                    if (connectionValue.isObject()) {
                        QJsonObject connectionObj = connectionValue.toObject();

                        LLMConnection connection;
                        connection.m_name = connectionObj["name"].toString();
                        connection.m_provider = connectionObj["provider"].toString();
                        connection.m_apiUrl = connectionObj["apiUrl"].toString();
                        connection.m_apiKey = connectionObj["apiKey"].toString();
                        int at = connectionObj["apiKey"].toInt(0);
                        if (at >= LLMConnection::AuthType::AuthToken && at <= LLMConnection::AuthType::AuthBearer) {
                            connection.m_authType = (LLMConnection::AuthType) at;
                        }
                        connection.m_isDefault = connectionObj["isDefault"].toBool(false);
                        connection.m_isEnabled = connectionObj["isEnabled"].toBool(true);

                        // Load endpoints map
                        if (connectionObj.contains("endpoints") && connectionObj["endpoints"].isObject()) {
                            QJsonObject endpointsObj = connectionObj["endpoints"].toObject();
                            for (auto it = endpointsObj.begin(); it != endpointsObj.end(); ++it) {
                                bool ok;
                                int endpointValue = it.key().toInt(&ok);
                                if (ok) {
                                    LLMConnection::Endpoints endpoint = static_cast<LLMConnection::Endpoints>(endpointValue);
                                    connection.m_endpoints[endpoint] = it.value().toString();
                                }
                            }
                        }

                        m_connections[connection.m_name] = connection;
                    }
                }
            }
        }
    } else {
        // If file doesn't exist, load default connections
        loadDefaultConnections();
    }
}

void LLMConnectionModel::saveConnections()
{
    QString fileName = getConfigFileName();
    if (fileName.isEmpty())
        return;

    QJsonObject rootObj;

    // Convert connections to JSON array
    QJsonArray connectionsArray;
    for (const auto &connection : m_connections.values()) {
        QJsonObject connectionObj;
        connectionObj["name"] = connection.name();
        connectionObj["provider"] = connection.provider();
        connectionObj["apiUrl"] = connection.apiUrl();
        connectionObj["apiKey"] = connection.apiKey();
        connectionObj["authType"] = connection.authType();
        connectionObj["isDefault"] = connection.isDefault();
        connectionObj["isEnabled"] = connection.isEnabled();

        // Save endpoints map
        QJsonObject endpointsObj;
        for (auto it = connection.m_endpoints.begin(); it != connection.m_endpoints.end(); ++it) {
            endpointsObj[QString::number(it.key())] = it.value();
        }
        connectionObj["endpoints"] = endpointsObj;

        connectionsArray.append(connectionObj);
    }

    rootObj["connections"] = connectionsArray;

    QJsonDocument doc(rootObj);

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

LLMConnection LLMConnectionModel::defaultConnection() const
{
    // find default connection
    foreach (auto &c, getAllConnections()) {
        if (c.isDefault()) {
            return c;
        }
    }
    return {};
}

void LLMConnectionModel::setDefaultConnection(const QString &name)
{
    // Clear previous default flag
    foreach (auto &key, m_connections.keys()) {
        LLMConnection c = m_connections[key];
        if (c.m_isDefault) {
            c.m_isDefault = false;
            m_connections[key] = c;
        }
    }

    // Set new default
    foreach (auto &key, m_connections.keys()) {
        if (key == name) {
            LLMConnection c = m_connections[key];
            c.m_isDefault = true;
            m_connections[key] = c;
            break;
        }
    }

    saveConnections();
}

QList<LLMConnection> LLMConnectionModel::getAllConnections() const
{
    return m_connections.values();
}

LLMConnection LLMConnectionModel::getConnection(const QString &name) const
{
    foreach (const auto &connection, m_connections.values()) {
        if (connection.m_name == name) {
            return connection;
        }
    }

    return LLMConnection();
}

void LLMConnectionModel::addConnection(const LLMConnection &connection)
{
    // Check if connection with this name already exists
    foreach (auto &nameKey, m_connections.keys()) {
        if (nameKey == connection.m_name) {
            // Update existing connection
            m_connections[nameKey] = connection;
            saveConnections();
            return;
        }
    }

    // Add new connection
    m_connections[connection.m_name] = connection;
    saveConnections();
}

void LLMConnectionModel::updateConnection(const QString &name, const LLMConnection &connection)
{
    foreach (auto &nameKey, m_connections.keys()) {
        if (nameKey == name) {
            m_connections[nameKey] = connection;
            saveConnections();
            return;
        }
    }
}

void LLMConnectionModel::removeConnection(const QString &name)
{
    foreach (auto &nameKey, m_connections.keys()) {
        if (nameKey == name) {
            m_connections.remove(nameKey);
            saveConnections();
            return;
        }
    }
}

QString LLMConnectionModel::getConfigFileName() const
{
    // Use the standard configuration path for macOS
    QDir configDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!configDir.exists()) {
        QFile::Permissions permissions;
        permissions.setFlag(QFile::Permission::ReadOwner, true);
        permissions.setFlag(QFile::Permission::ReadGroup, true);
        permissions.setFlag(QFile::Permission::WriteOwner, true);
        permissions.setFlag(QFile::Permission::WriteGroup, true);
        permissions.setFlag(QFile::Permission::ExeOwner, true);
        permissions.setFlag(QFile::Permission::ExeGroup, true);
        if (!configDir.mkpath(configDir.absolutePath(), permissions)) {
            qCritical("Unable to create directory: %s", qPrintable(configDir.absolutePath()));
            return QString();
        }
    }
    return configDir.absoluteFilePath("connections.json");
}

void LLMConnectionModel::loadDefaultConnections()
{
    LLMConnection data;

    data = LLMConnection( //
        "LLM-Studio Server",
        "LLM-Studio",
        "http://localhost:1234",
        "",
        true,
        true);
    data.setEndpointUri(LLMConnection::EndpointModels, "/v1/models");
    data.setEndpointUri(LLMConnection::EndpointCompletion, "/v1/chat/completions");
    m_connections["LLM-Studio Server"] = data;

    // /v1/chat/completions
    data = LLMConnection( //
        "llamacpp-server",
        "llamacpp",
        "http://localhost:8000",
        "",
        false,
        true);
    data.setEndpointUri(LLMConnection::EndpointModels, "/v1/models");
    data.setEndpointUri(LLMConnection::EndpointCompletion, "/v1/chat/completions");
    m_connections["llamacpp-server"] = data;

    // /v1/chat/completions
    data = LLMConnection( //
        "OpenAI GPT-4",
        "OpenAI",
        "https://api.openai.com",
        "",
        false,
        true);
    data.setEndpointUri(LLMConnection::EndpointModels, "/v1/models");
    data.setEndpointUri(LLMConnection::EndpointCompletion, "/v1/chat/completions");
    m_connections["OpenAI GPT-4"] = data;

    // /v1/chat/completions
    data = LLMConnection( //
        "OpenAI GPT-3.5",
        "OpenAI",
        "https://api.openai.com",
        "",
        false,
        true);
    data.setEndpointUri(LLMConnection::EndpointModels, "/v1/models");
    data.setEndpointUri(LLMConnection::EndpointCompletion, "/v1/chat/completions");
    m_connections["OpenAI GPT-3.5"] = data;

    // /v1/messages
    data = LLMConnection( //
        "Anthropic Claude",
        "Anthropic",
        "https://api.anthropic.com",
        "",
        false,
        true);
    data.setEndpointUri(LLMConnection::EndpointModels, "/v1/models");
    data.setEndpointUri(LLMConnection::EndpointCompletion, "/v1/messages");
    m_connections["Anthropic Claude"] = data;

    // /v1/chat/completions
    data = LLMConnection( //
        "Hugging Face",
        "Hugging Face",
        "https://api-inference.huggingface.co",
        "",
        false,
        true);
    data.setEndpointUri(LLMConnection::EndpointModels, "/v1/models");
    data.setEndpointUri(LLMConnection::EndpointCompletion, "/v1/chat/completions");
    m_connections["Hugging Face"] = data;

    saveConnections();
}

// QAbstractTableModel methods
int LLMConnectionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_connections.size();
}

int LLMConnectionModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return ColumnCount;
}

QVariant LLMConnectionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case NameColumn:
                return tr("Name");
            case ProviderColumn:
                return tr("Provider");
            case ApiUrlColumn:
                return tr("API URL");
            case IsDefaultColumn:
                return tr("Default");
            case IsEnabledColumn:
                return tr("Enabled");
            default:
                return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags LLMConnectionModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags flags = QAbstractTableModel::flags(index);

    // Make all columns editable except the default column
    if (index.column() != IsDefaultColumn) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}

QVariant LLMConnectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_connections.size())
        return QVariant();

    auto it = m_connections.begin();
    std::advance(it, index.row());
    const LLMConnection &connection = it.value();

    switch (role) {
        case Qt::UserRole: {
            return QVariant::fromValue<LLMConnection>(connection);
        }
        case NameRole: {
            return connection.m_name;
            break;
        }
        case ProviderRole: {
            return connection.m_provider;
            break;
        }
        case ApiUrlRole: {
            return connection.m_apiUrl;
            break;
        }
        case ApiKeyRole: {
            return connection.m_apiKey;
            break;
        }
        case AuthTypeRole: {
            return QVariant::fromValue<LLMConnection::AuthType>(connection.m_authType);
            break;
        }
        case IsEnabledRole: {
            return connection.m_isEnabled;
            break;
        }
        case IsDefaultRole: {
            return connection.m_isDefault;
            break;
        }
        case Qt::DisplayRole:
            switch (index.column()) {
                case NameColumn:
                    return connection.m_name;
                case ProviderColumn:
                    return connection.m_provider;
                case ApiUrlColumn:
                    return connection.m_apiUrl;
                case IsDefaultColumn:
                    return connection.m_isDefault ? "Yes" : "No";
                case IsEnabledColumn:
                    return connection.m_isEnabled ? "Yes" : "No";
                default:
                    return QVariant();
            }
#if 0
        case Qt::CheckStateRole:
            if (index.column() == IsDefaultColumn) {
                return connection.m_isDefault ? Qt::Checked : Qt::Unchecked;
            } else if (index.column() == IsEnabledColumn) {
                return connection.m_isEnabled ? Qt::Checked : Qt::Unchecked;
            }
            return QVariant();
#endif
        default:
            break;
    }
    return QVariant();
}

bool LLMConnectionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_connections.size())
        return false;

    auto it = m_connections.begin();
    std::advance(it, index.row());
    QString name = it.key();
    LLMConnection &connection = it.value();

    bool changed = false;

    switch (role) {
        case Qt::UserRole: {
            if (value.canConvert<LLMConnection>()) {
                connection = value.value<LLMConnection>();
                changed = true;
            }
        }
        case NameRole: {
            if (connection.m_name != value.toString()) {
                connection.m_name = value.toString();
                changed = true;
            }
            break;
        }
        case ProviderRole: {
            if (connection.m_provider != value.toString()) {
                connection.m_provider = value.toString();
                changed = true;
            }
            break;
        }
        case ApiUrlRole: {
            if (connection.m_apiUrl != value.toString()) {
                connection.m_apiUrl = value.toString();
                changed = true;
            }
            break;
        }
        case ApiKeyRole: {
            if (connection.m_apiKey != value.toString()) {
                connection.m_apiKey = value.toString();
                changed = true;
            }
            break;
        }
        case AuthTypeRole: {
            if (connection.m_authType != value.value<LLMConnection::AuthType>()) {
                connection.m_authType = value.value<LLMConnection::AuthType>();
                changed = true;
            }
            break;
        }
        case IsEnabledRole: {
            bool isEnabled = (value.toInt() == Qt::Checked);
            if (connection.m_isEnabled != isEnabled) {
                connection.m_isEnabled = isEnabled;
                changed = true;
            }
            break;
        }
        case IsDefaultRole: {
            bool isDefault = (value.toInt() == Qt::Checked);
            if (connection.m_isDefault != isDefault) {
                connection.m_isDefault = isDefault;
                changed = true;
            }
            break;
        }
    }

    if (changed) {
        // If this connection is being set as default, update the default connection name
        if (index.column() == IsDefaultRole && connection.m_isDefault) {
        }
        saveConnections();
        emit dataChanged(index, index, {role});
        return true;
    }

    return false;
}
