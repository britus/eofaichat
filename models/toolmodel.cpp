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

QVariant ToolModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_toolEntries.size())
        return QVariant();

    const ToolEntry &entry = m_toolEntries[index.row()];

    switch (role) {
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

void ToolModel::loadFromDirectory(const QString &directory, ToolType type)
{
    QDir dir(directory);
    if (!dir.exists()) {
        qWarning() << "[TOOLMODEL] Directory does not exist:" << directory;
        return;
    }

    // Find all JSON files in current directory
    QStringList jsonFiles;
    QFileInfoList fileInfoList = dir.entryInfoList( //
        QStringList() << "*.json",
        QDir::Files | QDir::NoDotAndDotDot,
        QDir::Name);
    foreach (const QFileInfo &fileInfo, fileInfoList) {
        jsonFiles.append(fileInfo.absoluteFilePath());
    }

    // Process JSON files
    for (const QString &filePath : jsonFiles) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QFileInfo fi(file.fileName());
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject jsonObject = doc.object();
                ToolEntry entry;
                entry.tool = jsonObject;
                entry.name = fi.baseName();
                entry.option = AskBeforeRun; // Default value
                entry.type = type;

                addToolEntry(entry);
            }
            file.close();
        }
    }

    // Recursively process subdirectories
    QFileInfoList subDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    foreach (const QFileInfo &subDirInfo, subDirs) {
        loadFromDirectory(subDirInfo.absoluteFilePath(), type);
    }
}

QList<QJsonObject> ToolModel::toolObjects() const
{
    QList<QJsonObject> result;
    for (const ToolEntry &entry : m_toolEntries) {
        result.append(entry.tool);
    }
    return result;
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
        QString configPath = baseCfgPath + QDir::separator() + "Tools";

        // ensure directory exist
        QDir configDir(configPath);
        createConfigDir(configDir);

        QDir::Filters filters = QDir::Files | QDir::Readable | QDir::AllDirs;
        configDir.setFilter(filters);

        // get tool configuration files
        QFileInfoList files = configDir.entryInfoList(QStringList() << "*.json", filters);
        foreach (auto fi, files) {
            if (fi.isFile() && fi.isReadable()) {
                loadFromDirectory(fi.absoluteFilePath(), Tool);
            }
        }
    }

    { // Known resources
        QString configPath = baseCfgPath + QDir::separator() + "Resource";

        // ensure directory exist
        QDir configDir(configPath);
        createConfigDir(configDir);

        QDir::Filters filters = QDir::Files | QDir::Readable | QDir::AllDirs;
        configDir.setFilter(filters);

        // get tool configuration files
        QFileInfoList files = configDir.entryInfoList(QStringList() << "*.json", filters);
        foreach (auto fi, files) {
            if (fi.isFile() && fi.isReadable()) {
                loadFromDirectory(fi.absoluteFilePath(), Resource);
            }
        }
    }

    { // Prompts
        QString configPath = baseCfgPath + QDir::separator() + "Prompts";

        // ensure directory exist
        QDir configDir(configPath);
        createConfigDir(configDir);

        QDir::Filters filters = QDir::Files | QDir::Readable | QDir::AllDirs;
        configDir.setFilter(filters);

        // get tool configuration files
        QFileInfoList files = configDir.entryInfoList(QStringList() << "*.json", filters);
        foreach (auto fi, files) {
            if (fi.isFile() && fi.isReadable()) {
                loadFromDirectory(fi.absoluteFilePath(), Prompt);
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
