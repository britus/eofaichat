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

        if (doc.isObject()) {
            QJsonObject rootObj = doc.object();

            // Load default connection name
            if (rootObj.contains("defaultConnection")) {
                m_defaultConnectionName = rootObj["defaultConnection"].toString();
            }

            // Load connections
            if (rootObj.contains("connections") && rootObj["connections"].isArray()) {
                QJsonArray connectionsArray = rootObj["connections"].toArray();

                for (const auto &connectionValue : connectionsArray) {
                    if (connectionValue.isObject()) {
                        QJsonObject connectionObj = connectionValue.toObject();

                        ConnectionData connection;
                        connection.m_name = connectionObj["name"].toString();
                        connection.m_provider = connectionObj["provider"].toString();
                        connection.m_apiUrl = connectionObj["apiUrl"].toString();
                        connection.m_apiKey = connectionObj["apiKey"].toString();
                        connection.m_isDefault = connectionObj["isDefault"].toBool(false);
                        connection.m_isEnabled = connectionObj["isEnabled"].toBool(true);

                        // Load endpoints map
                        if (connectionObj.contains("endpoints") && connectionObj["endpoints"].isObject()) {
                            QJsonObject endpointsObj = connectionObj["endpoints"].toObject();
                            for (auto it = endpointsObj.begin(); it != endpointsObj.end(); ++it) {
                                bool ok;
                                int endpointValue = it.key().toInt(&ok);
                                if (ok) {
                                    ConnectionData::Endpoints endpoint = static_cast<ConnectionData::Endpoints>(endpointValue);
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

    // Set default connection name
    rootObj["defaultConnection"] = m_defaultConnectionName;

    // Convert connections to JSON array
    QJsonArray connectionsArray;
    for (const auto &connection : m_connections.values()) {
        QJsonObject connectionObj;
        connectionObj["name"] = connection.name();
        connectionObj["provider"] = connection.provider();
        connectionObj["apiUrl"] = connection.apiUrl();
        connectionObj["apiKey"] = connection.apiKey();
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

QList<LLMConnectionModel::ConnectionData> LLMConnectionModel::getAllConnections() const
{
    return m_connections.values();
}

LLMConnectionModel::ConnectionData LLMConnectionModel::getConnection(const QString &name) const
{
    foreach (const auto &connection, m_connections.values()) {
        if (connection.m_name == name) {
            return connection;
        }
    }

    return ConnectionData();
}

void LLMConnectionModel::addConnection(const ConnectionData &connection)
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

void LLMConnectionModel::updateConnection(const QString &name, const ConnectionData &connection)
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
            // If this was the default connection, clear it
            if (m_defaultConnectionName == name) {
                m_defaultConnectionName.clear();
            }
            saveConnections();
            return;
        }
    }
}

void LLMConnectionModel::setDefaultConnection(const QString &name)
{
    // Clear previous default flag
    foreach (auto &nameKey, m_connections.keys()) {
        ConnectionData c = m_connections[nameKey];
        if (c.m_isDefault) {
            c.m_isDefault = false;
            m_connections[nameKey] = c;
        }
    }

    // Set new default
    foreach (auto &nameKey, m_connections.keys()) {
        if (nameKey == name) {
            ConnectionData c = m_connections[nameKey];
            c.m_isDefault = true;
            m_connections[nameKey] = c;
            break;
        }
    }

    m_defaultConnectionName = name;
    saveConnections();
}

QString LLMConnectionModel::getDefaultConnectionName() const
{
    return m_defaultConnectionName;
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
    ConnectionData data;

    data = ConnectionData( //
        "LLM-Studio Server",
        "LLM-Studio",
        "http://localhost:1234",
        "",
        true,
        true);
    data.setEndpointUri(ConnectionData::EndpointModels, "/v1/models");
    data.setEndpointUri(ConnectionData::EndpointCompletion, "/v1/chat/completions");
    m_connections["LLM-Studio Server"] = data;

    // /v1/chat/completions
    data = ConnectionData( //
        "llamacpp-server",
        "llamacpp",
        "http://localhost:8000",
        "",
        false,
        true);
    data.setEndpointUri(ConnectionData::EndpointModels, "/v1/models");
    data.setEndpointUri(ConnectionData::EndpointCompletion, "/v1/chat/completions");
    m_connections["llamacpp-server"] = data;

    // /v1/chat/completions
    data = ConnectionData( //
        "OpenAI GPT-4",
        "OpenAI",
        "https://api.openai.com",
        "",
        false,
        true);
    data.setEndpointUri(ConnectionData::EndpointModels, "/v1/models");
    data.setEndpointUri(ConnectionData::EndpointCompletion, "/v1/chat/completions");
    m_connections["OpenAI GPT-4"] = data;

    // /v1/chat/completions
    data = ConnectionData( //
        "OpenAI GPT-3.5",
        "OpenAI",
        "https://api.openai.com",
        "",
        false,
        true);
    data.setEndpointUri(ConnectionData::EndpointModels, "/v1/models");
    data.setEndpointUri(ConnectionData::EndpointCompletion, "/v1/chat/completions");
    m_connections["OpenAI GPT-3.5"] = data;

    // /v1/messages
    data = ConnectionData( //
        "Anthropic Claude",
        "Anthropic",
        "https://api.anthropic.com",
        "",
        false,
        true);
    data.setEndpointUri(ConnectionData::EndpointModels, "/v1/models");
    data.setEndpointUri(ConnectionData::EndpointCompletion, "/v1/messages");
    m_connections["Anthropic Claude"] = data;

    // /v1/chat/completions
    data = ConnectionData( //
        "Hugging Face",
        "Hugging Face",
        "https://api-inference.huggingface.co",
        "",
        false,
        true);
    data.setEndpointUri(ConnectionData::EndpointModels, "/v1/models");
    data.setEndpointUri(ConnectionData::EndpointCompletion, "/v1/chat/completions");
    m_connections["Hugging Face"] = data;

    m_defaultConnectionName = "LLM-Studio Server";
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

QVariant LLMConnectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_connections.size())
        return QVariant();

    auto it = m_connections.begin();
    std::advance(it, index.row());
    const ConnectionData &connection = it.value();

    switch (role) {
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
        case Qt::CheckStateRole:
            if (index.column() == IsDefaultColumn) {
                return connection.m_isDefault ? Qt::Checked : Qt::Unchecked;
            } else if (index.column() == IsEnabledColumn) {
                return connection.m_isEnabled ? Qt::Checked : Qt::Unchecked;
            }
            return QVariant();
        default:
            return QVariant();
    }
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

bool LLMConnectionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_connections.size())
        return false;

    auto it = m_connections.begin();
    std::advance(it, index.row());
    QString name = it.key();
    ConnectionData &connection = it.value();

    bool changed = false;

    switch (role) {
        case Qt::EditRole:
            switch (index.column()) {
                case NameColumn:
                    if (connection.m_name != value.toString()) {
                        connection.m_name = value.toString();
                        changed = true;
                    }
                    break;
                case ProviderColumn:
                    if (connection.m_provider != value.toString()) {
                        connection.m_provider = value.toString();
                        changed = true;
                    }
                    break;
                case ApiUrlColumn:
                    if (connection.m_apiUrl != value.toString()) {
                        connection.m_apiUrl = value.toString();
                        changed = true;
                    }
                    break;
                default:
                    return false;
            }
            break;
        case Qt::CheckStateRole:
            if (index.column() == IsDefaultColumn) {
                bool isDefault = (value.toInt() == Qt::Checked);
                if (connection.m_isDefault != isDefault) {
                    connection.m_isDefault = isDefault;
                    changed = true;
                }
            } else if (index.column() == IsEnabledColumn) {
                bool isEnabled = (value.toInt() == Qt::Checked);
                if (connection.m_isEnabled != isEnabled) {
                    connection.m_isEnabled = isEnabled;
                    changed = true;
                }
            }
            break;
        default:
            return false;
    }

    if (changed) {
        // If this connection is being set as default, update the default connection name
        if (index.column() == IsDefaultColumn && connection.m_isDefault) {
            m_defaultConnectionName = name;
        }

        saveConnections();
        emit dataChanged(index, index, {role});
        return true;
    }

    return false;
}
