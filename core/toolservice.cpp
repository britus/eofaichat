#include <toolservice.h>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

ToolService::ToolService(QObject *parent)
    : QObject{parent}
{
#define PARAM_SIG const ToolModel::ToolModelEntry &tool, const QJsonObject &args

    m_functions["display_project_files"] = [this](PARAM_SIG) -> QJsonObject const {
        return listDirectory(tool, args);
    };
    m_functions["list_source_files"] = [this](PARAM_SIG) -> QJsonObject const {
        return listDirectory(tool, args);
    };

    m_functions["read_source_file"] = [this](PARAM_SIG) -> QJsonObject const {
        QString filePath;
        if (args.contains("file_path") && args["file_path"].isString()) {
            filePath = args["file_path"].toString();
            return readSourceFile(filePath);
        } else {
            return createErrorResponse( //
                QStringLiteral("Parameter 'file_path' is missing in function: %1").arg(tool.name));
        }
    };

    m_functions["write_source_file"] = [this](PARAM_SIG) -> QJsonObject const {
        QString filePath;
        if (!args.contains("file_path") || !args["file_path"].isString()) {
            return createErrorResponse( //
                QStringLiteral("Parameter 'file_path' is missing in function: %1").arg(tool.name));
        }
        filePath = args["file_path"].toString();

        QString content;
        if (!args.contains("content") || !args["content"].isString()) {
            return createErrorResponse( //
                QStringLiteral("Parameter 'content' is missing in function: %1").arg(tool.name));
        }
        content = args["content"].toString();

        bool backup = true;
        if (!args.contains("content") || !args["content"].isBool()) {
            backup = true;
        } else {
            backup = args["create_backup"].toBool();
        }

        return writeSourceFile(filePath, content.toUtf8(), backup);
    };
}

QJsonObject ToolService::execute(const ToolModel::ToolModelEntry &tool, const QString &arguments) const
{
    qDebug().noquote() << "[ToolService] execute type:" << tool.type //
                       << "name:" << tool.name << "args:" << arguments;

    QJsonObject args;
    if (!arguments.isEmpty()) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(arguments.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            return createErrorResponse(tr("[ToolService] JSON error #%1 offset: %2 - %3") //
                                           .arg(error.error)
                                           .arg(error.offset)
                                           .arg(error.errorString()));
        }
        args = doc.object();
    }

    // if not listed, assume prompt or resource
    if (!m_functions.contains(tool.name)) {
        return tool.tool;
    }

    // execute tool function
    return m_functions[tool.name](tool, args);
}

QJsonObject ToolService::listDirectory(const ToolModel::ToolModelEntry &tool, const QJsonObject &args) const
{
    QJsonObject result;
    QStringList extensions;
    QString sortBy = "name";
    bool recursive = true;
    QString pathName;

    if (args.contains("project_path") && args["project_path"].isString()) {
        pathName = args["project_path"].toString();
    } else if (args.contains("directory") && args["directory"].isString()) {
        pathName = args["directory"].toString();
    }
    if (args.contains("recursive") && args["recursive"].isBool()) {
        recursive = args["recursive"].toBool();
    }
    if (args.contains("sortBy") && args["sortBy"].isString()) {
        sortBy = args["sortBy"].toString();
    }
    if (args.contains("extensions") && args["extensions"].isArray()) {
        extensions = getFileExtensions(args["extensions"].toArray());
    }
    if (extensions.isEmpty()) {
        extensions = DEFAULT_EXTENSIONS;
    }

    if (tool.name == "display_project_files") {
        return displayProjectFiles(pathName, recursive, sortBy, extensions);
    }

    if (tool.name == "list_source_files") {
        return listSourceFiles(pathName, extensions);
    }

    return {};
}

QJsonObject ToolService::displayProjectFiles(const QString &projectPath, bool recursive, const QString &sortBy, const QStringList extensions) const
{
    qDebug().noquote()                                              //
        << "[ToolService]:displayProjectFiles path:" << projectPath //
        << "sortBy:" << sortBy << "recusive:" << recursive << "extensions:" << extensions;

    if (projectPath.isEmpty()) {
        return createErrorResponse("Parameter 'project_path' or 'directory' required");
    }

    if (!isValidPath(projectPath)) {
        return createErrorResponse(QString("Invalid project path: %1").arg(projectPath));
    }

    QList<QFileInfo> fileList = findSourceFiles(projectPath, extensions, recursive);

    if (sortBy == "size") {
        std::sort(fileList.begin(), fileList.end(), [](const QFileInfo &a, const QFileInfo &b) { //
            return a.size() < b.size();
        });
    } else if (sortBy == "date") {
        std::sort(fileList.begin(), fileList.end(), [](const QFileInfo &a, const QFileInfo &b) { //
            return a.lastModified() < b.lastModified();
        });
    } else {
        std::sort(fileList.begin(), fileList.end(), [](const QFileInfo &a, const QFileInfo &b) { //
            return a.fileName() < b.fileName();
        });
    }

    QJsonObject structContent;
    QJsonArray jsonFiles;
    QSet<QString> directories;
    QStringList textLines;
    qint64 iTotalSize = 0;

    auto toTextLine = [](const QFileInfo &fileInfo, const QString &strBaseDir) -> QString { //
        QString filePath = fileInfo.absoluteFilePath();
        QString relativePath = ".";

        if (!strBaseDir.isEmpty()) {
            QDir baseDir(strBaseDir);
            relativePath = baseDir.relativeFilePath(fileInfo.absoluteFilePath());
        }

        QString size = QString::number(static_cast<int>(fileInfo.size()));
        QString lastModified = fileInfo.lastModified().toString(Qt::ISODate);
        QString directory = fileInfo.absolutePath();

        return QStringLiteral("%1|%2|%3|%4|%5").arg(filePath, size, lastModified, directory, relativePath);
    };

    foreach (const QFileInfo &fileInfo, fileList) {
        jsonFiles.append(fileInfoToJson(fileInfo, projectPath));
        textLines.append(toTextLine(fileInfo, projectPath));
        directories.insert(fileInfo.path());
        iTotalSize += fileInfo.size();
    }
    structContent["files"] = jsonFiles;

    QJsonObject jsonSummary;
    jsonSummary["project_path"] = projectPath;
    jsonSummary["total_files"] = static_cast<int>(fileList.size());
    jsonSummary["total_size"] = static_cast<int>(iTotalSize);
    jsonSummary["directories"] = static_cast<int>(directories.size());
    structContent["summary"] = jsonSummary;

    // result
    auto timestamp = QDateTime::currentDateTime().toString(Qt::ISODate) + "Z";

    QJsonDocument doc = QJsonDocument(jsonFiles);
    QJsonArray resultText = QJsonArray({QJsonObject({
        QPair<QString, QString>("type", "text"), //
        QPair<QString, QString>("text", doc.toJson(QJsonDocument::Compact) /*textLines.join("\n")*/),
    })});

    QJsonObject response = QJsonObject({
        QPair<QString, QJsonValue>("structuredContent", structContent),
        QPair<QString, QJsonValue>("content", QJsonArray({resultText})),
    });

    return response;
}

QJsonObject ToolService::listSourceFiles(const QString &projectPath, const QStringList &extensions) const
{
    qDebug().noquote()                            //
        << "[ToolService]:listSourceFiles: path:" //
        << projectPath << "extensions:" << extensions;

    if (projectPath.isEmpty()) {
        return createErrorResponse("Parameter 'project_path' or 'directory' required");
    }

    if (!isValidPath(projectPath)) {
        return createErrorResponse(QString("Invalid project path: %1").arg(projectPath));
    }

    QList<QFileInfo> fileList = findSourceFiles(projectPath, extensions, true);

    QJsonObject jsonResponse;
    QJsonArray jsonFiles;

    foreach (const QFileInfo &fileInfo, fileList) {
        jsonFiles.append(fileInfoToJson(fileInfo, projectPath));
    }

    // result
    auto timestamp = QDateTime::currentDateTime().toString(Qt::ISODate) + "Z";
    QJsonObject structContent = QJsonObject({
        QPair<QString, QJsonValue>("files", jsonFiles),
        QPair<QString, QJsonValue>("total_files", QJsonValue(static_cast<int>(fileList.size()))),
        QPair<QString, QJsonValue>("project_path", QJsonValue(projectPath)),
    });

    QJsonDocument doc = QJsonDocument(structContent);
    QJsonObject textResult = QJsonObject({
        QPair<QString, QString>("type", "text"), //
        QPair<QString, QString>("text", doc.toJson()),
    });

    QJsonObject response = QJsonObject({
        QPair<QString, QJsonValue>("structuredContent", structContent),
        QPair<QString, QJsonValue>("content", QJsonArray({textResult})),
    });

    return response;
}

QJsonObject ToolService::readSourceFile(const QString &filePath, qsizetype length, qsizetype offset) const
{
    qDebug() << "[ToolService]:readSourceFile: file:" << filePath;
    qDebug() << "[ToolService]:readSourceFile: offset:" << offset;
    qDebug() << "[ToolService]:readSourceFile: length:" << length;

    if (filePath.isEmpty()) {
        return createErrorResponse("Parameter 'file_path' required");
    }

    if (!isValidPath(filePath)) {
        return createErrorResponse(QString("Invalid file path: %1").arg(filePath));
    }

    QFile file(filePath);

    if (!file.exists()) {
        return createErrorResponse(QString("File not found: %1").arg(file.fileName()));
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return createErrorResponse(QString("File could not be opened: %1").arg(file.fileName()));
    }

    QByteArray byteContent = file.readAll();
    file.close();

    QString strContent;
    if (length <= 0) {
        strContent = QString::fromUtf8(byteContent);
    } else if (offset <= 0) {
        strContent = QString::fromUtf8(byteContent).left(length);
    } else {
        strContent = QString::fromUtf8(byteContent).mid(offset, length);
    }

    int iLineCount = strContent.count('\n') + (strContent.isEmpty() ? 0 : 1);

    QJsonObject structContent;
    structContent["file_path"] = file.fileName();
    structContent["content"] = strContent;
    structContent["encoding"] = "UTF-8";
    structContent["line_count"] = iLineCount;
    structContent["size"] = static_cast<int>(byteContent.size());

    // result
    //auto timestamp = QDateTime::currentDateTime().toString(Qt::ISODate) + "Z";

    QJsonDocument doc = QJsonDocument(structContent);
    QJsonObject textResult = QJsonObject({
        QPair<QString, QString>("type", "text"),       //
        QPair<QString, QString>("text", doc.toJson()), //
    });

    QJsonObject response = {
        QPair<QString, QJsonValue>("structuredContent", structContent),
        QPair<QString, QJsonValue>("content", QJsonArray({textResult})),
    };

    return response;
}

QJsonObject ToolService::writeSourceFile(const QString &filePath, const QByteArray &content, bool createBackup) const
{
    qDebug().noquote()                               //
        << "[ToolService]:writeSourceFile filePath:" //
        << filePath << "backup:" << createBackup;

    if (filePath.isEmpty()) {
        return createErrorResponse("Parameter 'filePath' required");
    }
    if (content.isEmpty()) {
        return createErrorResponse("Parameter 'content' required");
    }

    if (!isValidPath(filePath)) {
        return createErrorResponse(QString("Invalid file path: %1").arg(filePath));
    }
    if (content.isEmpty()) {
        return createErrorResponse(QString("No content in file: %1").arg(filePath));
    }

    QFileInfo fi(filePath);
    QDir tdir(fi.path());
    if (!tdir.exists()) {
        QFile::Permissions permissions;
        permissions.setFlag(QFile::Permission::ReadOwner, true);
        permissions.setFlag(QFile::Permission::ReadGroup, true);
        permissions.setFlag(QFile::Permission::WriteOwner, true);
        permissions.setFlag(QFile::Permission::WriteGroup, true);
        permissions.setFlag(QFile::Permission::ExeOwner, true);
        permissions.setFlag(QFile::Permission::ExeGroup, true);
        if (!tdir.mkpath(fi.path(), permissions)) {
            return createErrorResponse(QString("Unable to create directory: %1").arg(fi.path()));
        }
    }

    QJsonObject result;
    result["file_path"] = filePath;
    result["success"] = false;

    QString backupPath;
    if (createBackup && QFile::exists(filePath)) {
        backupPath = createBackupPath(filePath);
        if (!backupPath.isEmpty()) {
            result["backup_path"] = backupPath;
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result["message"] = QString("Error: File could not be written - %1").arg(file.errorString());
        return result;
    }

    QByteArray byteContent = content;
    qint64 bytesWritten = file.write(byteContent);
    file.close();

    if (bytesWritten == -1) {
        result["message"] = "Error writing the file";
        return result;
    }

    result["success"] = true;
    result["bytes_written"] = static_cast<int>(bytesWritten);
    result["message"] = QString("File successfully saved - %1 Bytes written").arg(bytesWritten);

    // result
    auto timestamp = QDateTime::currentDateTime().toString(Qt::ISODate) + "Z";

    QJsonDocument doc = QJsonDocument(result);
    QJsonArray resp_content = QJsonArray({QJsonObject({
        QPair<QString, QString>("type", "text"), //
        QPair<QString, QString>("text", doc.toJson()),
    })});

    QJsonObject response = QJsonObject({
        QPair<QString, QJsonValue>("structuredContent", result),
        QPair<QString, QJsonValue>("content", resp_content),
    });

    return response;
}

// ---------------------------------------------------------
// Private stuff
// ---------------------------------------------------------

QList<QFileInfo> ToolService::findSourceFiles(const QString &strPath, const QStringList &extensions, bool bRecursive) const
{
    QList<QFileInfo> fileList;
    QDir dir(strPath);

    if (!dir.exists()) {
        return fileList;
    }

    QDir::Filters filters = QDir::Files | QDir::Readable;
    if (bRecursive) {
        filters |= QDir::AllDirs;
    }
    dir.setFilter(filters);

    QStringList extList;
    if (extensions.isEmpty()) {
        extList = DEFAULT_EXTENSIONS;
    } else {
        extList = extensions;
    }

    QFileInfoList dirList = dir.entryInfoList();
    foreach (const QFileInfo &fileInfo, dirList) {
        if (fileInfo.isDir()) {
            if (bRecursive                           //
                && fileInfo.fileName() != "."        //
                && fileInfo.fileName() != ".."       //
                && fileInfo.fileName() != "build"    // QT/CMAKE build directory
                && fileInfo.fileName() != "bin"      // Java binaries
                && fileInfo.fileName() != "classes") // Java binaries
            {
                fileList.append(findSourceFiles(fileInfo.filePath(), extList, bRecursive));
            }
        } else {
            foreach (const QString &strExt, extList) {
                if (fileInfo.suffix() == strExt.mid(1)) {
                    fileList.append(fileInfo);
                    break;
                }
            }
        }
    }

    return fileList;
}

bool ToolService::isValidPath(const QString &strPath) const
{
    if (strPath.isEmpty()) {
        return false;
    }

    QFileInfo fileInfo(strPath);
    QString strAbsPath = fileInfo.absoluteFilePath();

    return !strAbsPath.isEmpty();
}

QString ToolService::createBackupPath(const QString &strOriginalPath) const
{
    QFileInfo fileInfo(strOriginalPath);
    QString strBackupDir = fileInfo.absolutePath();
    QString strBackupName = "_backup_"                                                 //
                            + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") //
                            + "_" + fileInfo.baseName() + "."                          //
                            + fileInfo.suffix() + ".txt";
    QString strBackupPath = strBackupDir + "/" + strBackupName;

    if (QFile::copy(strOriginalPath, strBackupPath)) {
        return strBackupPath;
    }

    return QString();
}

QJsonObject ToolService::fileInfoToJson(const QFileInfo &fileInfo, const QString &strBaseDir) const
{
    QJsonObject jsonFileInfo;
    jsonFileInfo["path"] = fileInfo.absoluteFilePath();

    if (!strBaseDir.isEmpty()) {
        QDir baseDir(strBaseDir);
        jsonFileInfo["relative_path"] = baseDir.relativeFilePath(fileInfo.absoluteFilePath());
    }

    jsonFileInfo["size"] = static_cast<int>(fileInfo.size());
    jsonFileInfo["last_modified"] = fileInfo.lastModified().toString(Qt::ISODate);
    jsonFileInfo["directory"] = fileInfo.absolutePath();

    return jsonFileInfo;
}

QStringList ToolService::getFileExtensions(const QJsonArray &jsonArray) const
{
    QStringList extensions;

    if (!jsonArray.isEmpty()) {
        for (const QJsonValue &value : jsonArray) {
            if (value.isString()) {
                QString strExt = value.toString();
                if (!strExt.startsWith(".")) {
                    strExt = "." + strExt;
                }
                extensions.append(strExt);
            }
        }
    } else {
        foreach (const auto &ext, DEFAULT_EXTENSIONS) {
            extensions.append(ext);
        }
    }

    return extensions;
}

QJsonObject ToolService::createErrorResponse(const QString &strErrorMsg) const
{
    QJsonObject jsonError;
    jsonError["success"] = false;
    jsonError["error"] = strErrorMsg;
    return jsonError;
}
