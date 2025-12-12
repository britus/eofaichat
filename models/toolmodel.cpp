#include "toolmodel.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

ToolModel::ToolModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int ToolModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_toolEntries.count();
}

bool ToolModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_toolEntries.size())
        return false;

    beginResetModel();
    ToolEntry &entry = m_toolEntries[index.row()];
    switch (role) {
        case Qt::UserRole:
            entry = value.value<ToolEntry>();
        case ToolRole:
            entry.tool = value.value<QJsonObject>();
        case NameRole:
            entry.name = value.value<QString>();
        case OptionRole:
            entry.option = value.value<ToolOption>();
        case TypeRole:
            entry.type = value.value<ToolType>();
        default:
            return false;
    }
    m_toolEntries[index.row()] = entry;
    endResetModel();
}

QVariant ToolModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_toolEntries.size())
        return QVariant();

    const ToolEntry &entry = m_toolEntries[index.row()];

    switch (role) {
        case Qt::UserRole:
            return QVariant::fromValue(entry);
        case ToolRole:
            return entry.tool;
        case NameRole:
            return entry.name;
        case OptionRole:
            return entry.option;
        case TypeRole:
            return entry.type;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> ToolModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ToolRole] = "tool";
    roles[NameRole] = "name";
    roles[OptionRole] = "option";
    roles[TypeRole] = "type";
    return roles;
}

void ToolModel::loadFromDirectory(const QFileInfo &fileInfo, ToolType type)
{
    QFile file(fileInfo.absoluteFilePath());
    if (file.open(QIODevice::ReadOnly)) {
        QFileInfo fi(file.fileName());
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject jsonObject = doc.object();
            ToolEntry entry;
            entry.tool = jsonObject;
            entry.name = fi.baseName();
            entry.option = ToolDisabled; // Default value
            entry.type = type;

            addToolEntry(entry);
        } else {
            qWarning().noquote() << "[ToolModel] loadFromDirectory:" //
                                 << error.errorString()              //
                                 << "Invalid JSON file:" << file.fileName();
        }
        file.close();
    }
}

QList<QJsonObject> ToolModel::toolObjects() const
{
    QList<QJsonObject> result;
    for (const ToolEntry &entry : m_toolEntries) {
        if (entry.option == ToolEnabled || entry.option == AskBeforeRun) {
            result.append(entry.tool);
        }
    }
    return result;
}

ToolModel::ToolEntry ToolModel::toolByName(const QString &name) const
{
    foreach (const ToolEntry &entry, m_toolEntries) {
        QString ename = entry.name;
        ename = ename.replace("-", "_");
        ename = ename.replace(" ", "_");
        ename = ename.toLower().trimmed();

        QString rname = name;
        rname = rname.replace("-", "_");
        rname = rname.replace(" ", "_");
        rname = rname.toLower().trimmed();

        if (rname.contains(ename)) {
            return entry;
        }
    }
    return {};
}

QJsonObject ToolModel::toolObject(const QString &name) const
{
    for (const ToolEntry &entry : m_toolEntries) {
        if (entry.name == name) {
            return entry.tool;
        }
    }
    return QJsonObject();
}

void ToolModel::addToolEntry(const ToolEntry &entry)
{
    int index = m_toolEntries.size();
    beginInsertRows(QModelIndex(), index, index);
    m_toolEntries.append(entry);
    endInsertRows();

    emit toolAdded(entry);
}

void ToolModel::removeToolEntry(int index)
{
    if (index < 0 || index >= m_toolEntries.size()) {
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_toolEntries.removeAt(index);
    endRemoveRows();

    emit toolRemoved(index);
}

inline void ToolModel::createConfigDir(const QDir &dir)
{
    if (!dir.exists()) {
        QFile::Permissions permissions;
        permissions.setFlag(QFile::Permission::ReadOwner, true);
        permissions.setFlag(QFile::Permission::ReadGroup, true);
        permissions.setFlag(QFile::Permission::WriteOwner, true);
        permissions.setFlag(QFile::Permission::WriteGroup, true);
        permissions.setFlag(QFile::Permission::ExeOwner, true);
        permissions.setFlag(QFile::Permission::ExeGroup, true);
        if (!dir.mkpath(dir.absolutePath(), permissions)) {
            qWarning("Unable to create directory: %s", qPrintable(dir.absolutePath()));
            return;
        }
    }
}

void ToolModel::loadToolsConfig()
{
    // server tools configuration
    QString baseCfgPath;
    baseCfgPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    { // Executable tools
        //QString configPath = baseCfgPath + QDir::separator() + "Tools";
        QString configPath = QStringLiteral(":/cfg") + QDir::separator() + QStringLiteral("Tools");

        // ensure directory exist
        QDir configDir(configPath);
        //createConfigDir(configDir);

        QDir::Filters filters = QDir::Files | QDir::Readable | QDir::AllDirs;
        configDir.setFilter(filters);

        // get tool configuration files
        QFileInfoList files = configDir.entryInfoList(QStringList() << "*.json", filters);
        foreach (auto fi, files) {
            if (fi.isFile() && fi.isReadable()) {
                loadFromDirectory(fi, Tool);
            }
        }
    }

    { // Known resources
        //QString configPath = baseCfgPath + QDir::separator() + "Resources";
        QString configPath = QStringLiteral(":/cfg") + QDir::separator() + QStringLiteral("Resources");

        // ensure directory exist
        QDir configDir(configPath);
        createConfigDir(configDir);

        QDir::Filters filters = QDir::Files | QDir::Readable | QDir::AllDirs;
        configDir.setFilter(filters);

        // get tool configuration files
        QFileInfoList files = configDir.entryInfoList(QStringList() << "*.json", filters);
        foreach (auto fi, files) {
            if (fi.isFile() && fi.isReadable()) {
                loadFromDirectory(fi, Resource);
            }
        }
    }

    { // Prompts
        //QString configPath = baseCfgPath + QDir::separator() + "Prompts";
        QString configPath = QStringLiteral(":/cfg") + QDir::separator() + QStringLiteral("Prompts");

        // ensure directory exist
        QDir configDir(configPath);
        createConfigDir(configDir);

        QDir::Filters filters = QDir::Files | QDir::Readable | QDir::AllDirs;
        configDir.setFilter(filters);

        // get tool configuration files
        QFileInfoList files = configDir.entryInfoList(QStringList() << "*.json", filters);
        foreach (auto fi, files) {
            if (fi.isFile() && fi.isReadable()) {
                loadFromDirectory(fi, Prompt);
            }
        }
    }
}

bool ToolModel::hasExecutables() const
{
    for (const ToolEntry &entry : m_toolEntries) {
        if (entry.type == Tool)
            return true;
    }
    return false;
}

bool ToolModel::hasResources() const
{
    for (const ToolEntry &entry : m_toolEntries) {
        if (entry.type == Resource)
            return true;
    }
    return false;
}

bool ToolModel::hasPrompts() const
{
    for (const ToolEntry &entry : m_toolEntries) {
        if (entry.type == Prompt)
            return true;
    }
    return false;
}
