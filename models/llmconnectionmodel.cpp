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
                        connection.name = connectionObj["name"].toString();
                        connection.provider = connectionObj["provider"].toString();
                        connection.apiUrl = connectionObj["apiUrl"].toString();
                        connection.apiKey = connectionObj["apiKey"].toString();
                        connection.isDefault = connectionObj["isDefault"].toBool(false);
                        connection.isEnabled = connectionObj["isEnabled"].toBool(true);

                        m_connections[connection.name] = connection;
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
        connectionObj["name"] = connection.name;
        connectionObj["provider"] = connection.provider;
        connectionObj["apiUrl"] = connection.apiUrl;
        connectionObj["apiKey"] = connection.apiKey;
        connectionObj["isDefault"] = connection.isDefault;
        connectionObj["isEnabled"] = connection.isEnabled;
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
        if (connection.name == name) {
            return connection;
        }
    }

    return ConnectionData();
}

void LLMConnectionModel::addConnection(const ConnectionData &connection)
{
    // Check if connection with this name already exists
    foreach (auto &nameKey, m_connections.keys()) {
        if (nameKey == connection.name) {
            // Update existing connection
            m_connections[nameKey] = connection;
            saveConnections();
            return;
        }
    }

    // Add new connection
    m_connections[connection.name] = connection;
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
        if (c.isDefault) {
            c.isDefault = false;
            m_connections[nameKey] = c;
        }
    }

    // Set new default
    foreach (auto &nameKey, m_connections.keys()) {
        if (nameKey == name) {
            ConnectionData c = m_connections[nameKey];
            c.isDefault = true;
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
    m_connections["LLM-Studio Server"] = {
        .name = "LLM-Studio Server",
        .provider = "LLM-Studio",
        .apiUrl = "http://localhost:1234/v1/chat/completions",
        .apiKey = "",
        .isDefault = true,
        .isEnabled = true,
    };

    m_connections["llamacpp-server"] = {
        .name = "llamacpp-server",
        .provider = "llamacpp",
        .apiUrl = "http://localhost:8000/v1/chat/completions",
        .apiKey = "",
        .isDefault = false,
        .isEnabled = true,
    };

    m_connections["OpenAI GPT-4"] = {
        .name = "OpenAI GPT-4",
        .provider = "OpenAI",
        .apiUrl = "https://api.openai.com/v1/chat/completions",
        .apiKey = "",
        .isDefault = false,
        .isEnabled = true,
    };

    m_connections["OpenAI GPT-3.5"] = {
        .name = "OpenAI GPT-3.5",
        .provider = "OpenAI",
        .apiUrl = "https://api.openai.com/v1/chat/completions",
        .apiKey = "",
        .isDefault = false,
        .isEnabled = true,
    };

    m_connections["Anthropic Claude"] = {
        .name = "Anthropic Claude",
        .provider = "Anthropic",
        .apiUrl = "https://api.anthropic.com/v1/messages",
        .apiKey = "",
        .isDefault = false,
        .isEnabled = true,
    };

    m_connections["Hugging Face"] = {
        .name = "Hugging Face",
        .provider = "Hugging Face",
        .apiUrl = "https://api-inference.huggingface.co/v1/chat/completions",
        .apiKey = "",
        .isDefault = false,
        .isEnabled = true,
    };

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
            return connection.name;
        case ProviderColumn:
            return connection.provider;
        case ApiUrlColumn:
            return connection.apiUrl;
        case IsDefaultColumn:
            return connection.isDefault ? "Yes" : "No";
        case IsEnabledColumn:
            return connection.isEnabled ? "Yes" : "No";
        default:
            return QVariant();
        }
    case Qt::CheckStateRole:
        if (index.column() == IsDefaultColumn) {
            return connection.isDefault ? Qt::Checked : Qt::Unchecked;
        } else if (index.column() == IsEnabledColumn) {
            return connection.isEnabled ? Qt::Checked : Qt::Unchecked;
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
            if (connection.name != value.toString()) {
                connection.name = value.toString();
                changed = true;
            }
            break;
        case ProviderColumn:
            if (connection.provider != value.toString()) {
                connection.provider = value.toString();
                changed = true;
            }
            break;
        case ApiUrlColumn:
            if (connection.apiUrl != value.toString()) {
                connection.apiUrl = value.toString();
                changed = true;
            }
            break;
        case IsEnabledColumn:
            if (connection.isEnabled != value.toBool()) {
                connection.isEnabled = value.toBool();
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
            if (connection.isDefault != isDefault) {
                connection.isDefault = isDefault;
                changed = true;
            }
        } else if (index.column() == IsEnabledColumn) {
            bool isEnabled = (value.toInt() == Qt::Checked);
            if (connection.isEnabled != isEnabled) {
                connection.isEnabled = isEnabled;
                changed = true;
            }
        }
        break;
    default:
        return false;
    }

    if (changed) {
        // If this connection is being set as default, update the default connection name
        if (index.column() == IsDefaultColumn && connection.isDefault) {
            m_defaultConnectionName = name;
        }
        
        saveConnections();
        emit dataChanged(index, index, {role});
        return true;
    }
    
    return false;
}