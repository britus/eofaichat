#include "toolmodel.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

    ToolModelEntry &entry = m_toolEntries[index.row()];
    switch (role) {
        case Qt::UserRole:
            entry = value.value<ToolModelEntry>();
        case ToolRole:
            entry.tool = value.value<QJsonObject>();
        case NameRole:
            entry.name = value.value<QString>();
        case OptionRole:
            entry.option = value.value<ToolOption>();
        case TypeRole:
            entry.type = value.value<ToolModelType>();
        case DescriptionRole:
            entry.description = value.value<QString>();
        case TitleRole:
            entry.title = value.value<QString>();
        case ExecHandlerRole:
            entry.execHandler = value.value<QString>();
        case ExecMethodRole:
            entry.execMethod = value.value<QString>();
        default: {
            return false;
        }
    }

    beginResetModel();
    m_toolEntries[index.row()] = entry;
    endResetModel();
}

QVariant ToolModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_toolEntries.size())
        return QVariant();

    const ToolModelEntry &entry = m_toolEntries[index.row()];

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
        case DescriptionRole:
            return entry.description;
        case TitleRole:
            return entry.title;
        case ExecHandlerRole:
            return entry.execHandler;
        case ExecMethodRole:
            return entry.execMethod;
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
    roles[DescriptionRole] = "description";
    roles[TitleRole] = "title";
    roles[ExecHandlerRole] = "execHandler";
    roles[ExecMethodRole] = "execMethod";
    return roles;
}

void ToolModel::loadFromDirectory(const QFileInfo &fileInfo, ToolModelType type)
{
    QFile file(fileInfo.absoluteFilePath());
    if (file.open(QIODevice::ReadOnly)) {
        QFileInfo fi(file.fileName());
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject jsonObject = doc.object();
            ToolModelEntry entry;
            entry.tool = jsonObject;
            entry.name = jsonObject["name"].toString(fi.baseName());
            entry.title = jsonObject["title"].toString(fi.baseName());
            entry.description = jsonObject["description"].toString(fi.baseName());
            entry.execHandler = jsonObject["execHandler"].toString("");
            entry.execMethod = jsonObject["execMethod"].toString("");
            entry.option = ToolDisabled; // Default value
            entry.type = type;
            addToolEntry(entry);
        } else {
            qWarning().noquote() << "[ToolModel] loadFromDirectory:"                //
                                 << "Invalid JSON file:" << file.fileName() << "\n" //
                                 << error.errorString();
        }
        file.close();
    }
}

QList<QJsonObject> ToolModel::toolObjects() const
{
    QList<QJsonObject> result;
    for (const ToolModelEntry &entry : m_toolEntries) {
        if (entry.option == ToolEnabled || entry.option == AskBeforeRun) {
            result.append(entry.tool);
        }
    }
    return result;
}

static inline bool isSameName(const QString &name1, const QString &name2)
{
    QString ename = name1;
    ename = ename.replace("-", "_");
    ename = ename.replace(" ", "_");
    ename = ename.toLower().trimmed();

    QString rname = name2;
    rname = rname.replace("-", "_");
    rname = rname.replace(" ", "_");
    rname = rname.toLower().trimmed();

    return rname.contains(ename);
}

ToolModel::ToolModelEntry ToolModel::toolByName(const QString &name) const
{
    foreach (const ToolModelEntry &entry, m_toolEntries) {
        if (isSameName(entry.name, name)) {
            return entry;
        }
    }
    return {};
}

QJsonObject ToolModel::toolObject(const QString &name) const
{
    for (const ToolModelEntry &entry : m_toolEntries) {
        if (isSameName(entry.name, name)) {
            return entry.tool;
        }
    }
    return QJsonObject();
}

void ToolModel::addToolEntry(const ToolModelEntry &entry)
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

inline bool ToolModel::createDirectory(const QDir &dir) const
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
            return false;
        }
    }
    return true;
}

inline QDir ToolModel::configDirectory(const QString &pathName) const
{
    // server tools configuration
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));

    // fallback to internal resource
    if (!QFileInfo::exists(dir.absoluteFilePath(pathName))) {
        if (!createDirectory(dir.absoluteFilePath(pathName))) {
            return QDir(QStringLiteral(":/cfg")).absoluteFilePath(pathName);
        }
    }

    return dir.absoluteFilePath(pathName);
}

bool ToolModel::deployResourceFiles(const QString &resourcePath, const QDir &targetDir)
{
    // Ensure target directory exists
    if (!targetDir.exists()) {
        if (!createDirectory(targetDir)) {
            qCritical() << "[ToolModel] Failed to create target directory:" << targetDir.absolutePath();
            return false;
        }
    }

    bool success = true;

    // Iterate through each subdirectory
    QDir sourceDir(resourcePath);
    QFileInfo sourceInfo(sourceDir.absolutePath());

    // Check if the source subdirectory exists
    if (!sourceDir.exists()) {
        qCritical() << "[ToolModel] Source subdirectory does not exist:" << sourceDir.absolutePath();
        return false;
    }

    // Get all files in the subdirectory
    QFileInfoList fileInfoList = sourceDir.entryInfoList( //
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    foreach (const QFileInfo &fileInfo, fileInfoList) {
        QString fileName = fileInfo.fileName();
        QString sourceFilePath = fileInfo.absoluteFilePath();
        QString targetFilePath = targetDir.absoluteFilePath(fileName);

        if (fileInfo.isDir()) {
            // Handle directories recursively
            QDir targetSubDir(targetFilePath);
            if (!targetSubDir.exists()) {
                if (!targetSubDir.mkpath(".")) {
                    qWarning() << "Failed to create directory:" << targetFilePath;
                    success = false;
                    continue;
                }
            }

            // Copy directory contents recursively
            if (!copyDirectoryRecursively(sourceFilePath, targetFilePath)) {
                qWarning() << "Failed to copy directory:" << sourceFilePath;
                success = false;
            }
        } else {
            // Handle files
            QFile sourceFile(sourceFilePath);
            QFileInfo sfi(sourceFile.fileName());
            QFile targetFile(targetFilePath);
            QFileInfo tfi(targetFile.fileName());

            // skip existing newer files
            if (sfi.lastModified() < tfi.lastModified()) {
                continue;
            }

            // Remove existing file if it exists
            if (targetFile.exists()) {
                if (!targetFile.remove()) {
                    qWarning() << "Failed to remove existing file:" << targetFilePath;
                    success = false;
                    continue;
                }
            }

            // Copy file
            if (!sourceFile.copy(targetFilePath)) {
                qWarning() << "Failed to copy file from" << sourceFilePath << "to" << targetFilePath;
                success = false;
            } else {
                qDebug() << "Copied file:" << fileName;
            }
        }
    }

    return success;
}

// Helper function to copy directories recursively
bool ToolModel::copyDirectoryRecursively(const QString &sourceDirPath, const QString &targetDirPath)
{
    QDir sourceDir(sourceDirPath);
    QDir targetDir(targetDirPath);

    if (!targetDir.exists()) {
        if (!targetDir.mkpath(".")) {
            return false;
        }
    }

    QFileInfoList fileInfoList = sourceDir.entryInfoList( //
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    foreach (const QFileInfo &fileInfo, fileInfoList) {
        QString fileName = fileInfo.fileName();
        QString sourceFilePath = fileInfo.absoluteFilePath();
        QString targetFilePath = targetDirPath + QDir::separator() + fileName;

        if (fileInfo.isDir()) {
            // Recursively copy subdirectory
            if (!copyDirectoryRecursively(sourceFilePath, targetFilePath)) {
                return false;
            }
        } else {
            // Copy file
            QFile sourceFile(sourceFilePath);
            QFile targetFile(targetFilePath);

            if (targetFile.exists()) {
                if (!targetFile.remove()) {
                    return false;
                }
            }

            if (!sourceFile.copy(targetFilePath)) {
                return false;
            }
        }
    }

    return true;
}

inline void ToolModel::loadToolsConfig(const QString &subPath, ToolModelType type)
{
    QDir::Filters filters = QDir::Files | QDir::Readable | QDir::AllDirs | QDir::NoDotAndDotDot;
    QDir configDir = configDirectory(subPath);
    configDir.setFilter(filters);

    // write internal tools configuation to app configuration directory
    if (!configDir.absolutePath().startsWith(":")) {
        if (!deployResourceFiles(QStringLiteral(":/cfg/%1").arg(subPath), configDir)) {
            qCritical("Unable to deploy tooling: %s", qPrintable(configDir.absolutePath()));
            return;
        }
    }

    // get tool configuration files
    QFileInfoList files = configDir.entryInfoList(QStringList() << "*.json", filters);
    foreach (auto fi, files) {
        if (fi.isFile() && fi.isReadable()) {
            loadFromDirectory(fi, type);
        }
    }
}

void ToolModel::loadToolsConfig()
{
    // Executable tools
    loadToolsConfig(QStringLiteral("Tools"), ToolModel::ToolFunction);
    // Known resources
    loadToolsConfig(QStringLiteral("Resources"), ToolModel::ToolResource);
    // Prompts
    loadToolsConfig(QStringLiteral("Prompts"), ToolModel::ToolPrompt);
}

bool ToolModel::hasExecutables() const
{
    for (const ToolModelEntry &entry : m_toolEntries) {
        if (entry.type == ToolModelType::ToolFunction)
            return true;
    }
    return false;
}

bool ToolModel::hasResources() const
{
    for (const ToolModelEntry &entry : m_toolEntries) {
        if (entry.type == ToolModelType::ToolResource)
            return true;
    }
    return false;
}

bool ToolModel::hasPrompts() const
{
    for (const ToolModelEntry &entry : m_toolEntries) {
        if (entry.type == ToolModelType::ToolPrompt)
            return true;
    }
    return false;
}
