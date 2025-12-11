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
    //
}

static inline const QJsonObject errorToJsonObject(int code, const QString &message)
{
    QJsonObject errorObj;
    errorObj.insert("code", QJsonValue(code));
    errorObj.insert("message", QJsonValue(message));
    return errorObj;
}

static inline const QByteArray errorToJson(const QJsonObject &error)
{
    QJsonDocument doc(error);
    return doc.toJson(QJsonDocument::Compact);
}

static inline void listDirectory(const QString &path, const QStringList &extensions, QByteArray &content)
{
    QDir dir(path);
    if (!dir.exists()) {
        content.append(errorToJson(errorToJsonObject( //
            -1,
            QStringLiteral("Directory '%1' does not exist.").arg(path).toUtf8())));
        return;
    }
    /*
  "outputSchema": {
    "type": "object",
    "properties": {
      "files": {
        "type": "array",
        "items": {
          "type": "object",
          "properties": {
            "path": {
              "type": "string",
              "description": "Absolute path to the file"
            },
            "relative_path": {
              "type": "string",
              "description": "Relative path from the project directory"
            },
            "size": {
              "type": "integer",
              "description": "File size in bytes"
            },
            "last_modified": {
              "type": "string",
              "description": "Last modified date (ISO 8601 format)"
            }
          }
        },
        "description": "List of found source code files"
      },
      "total_files": {
        "type": "integer",
        "description": "Total number of files found"
      },
      "project_path": {
        "type": "string",
        "description": "The project directory used"
      }
    },

*/
    QJsonArray entries;
    QList<QFileInfo> entryList = dir.entryInfoList();
    foreach (auto fi, entryList) {
        QJsonObject entry;
        entry["path"] = QJsonValue(fi.absoluteFilePath());
        entry["relative_path"] = QJsonValue(fi.canonicalFilePath());
        entry["size"] = QJsonValue(fi.size());
        entry["last_modified"] = QJsonValue(fi.lastModified().toString("dd.MM.yyy hh:mm:ss"));
        entries.append(entry);
    }
    QJsonObject files;
    files["files"] = entries;
    QJsonDocument doc(files);
    content.append(doc.toJson(QJsonDocument::Compact));
}

static inline void readFile(const QString &path, QByteArray &content)
{
    if (!QFile::exists(path)) {
        content.append(errorToJson(errorToJsonObject( //
            -1,
            QStringLiteral("Directory '%1' does not exist.").arg(path).toUtf8())));
        return;
    }
    /*
    "outputSchema": {
        "type": "object",
        "description": "object",
        "properties": {
            "file_path": {
                "type": "string",
                "description": "The path used for the file"
            },
            "content": {
                "type": "string",
                "description": "The full content of the file"
            },
            "encoding": {
                "type": "string",
                "description": "Character encoding of the file (e.g. UTF-8)"
            },
            "line_count": {
                "type": "integer",
                "description": "Number of lines in the file"
            },
            "size": {
                "type": "integer",
                "description": "File size in bytes"
            }
        },
        "required": ["file_path", "content", "encoding", "line_count", "size"]
    }
    */
    QFile file(path);
    if (!file.open(QFile::ReadOnly)) {
        content.append(errorToJson(errorToJsonObject( //
            -1,
            QStringLiteral("Can't find file %1").arg(path).toUtf8())));
        return;
    }

    QJsonObject result;
    result["file_path"] = QJsonValue(file.fileName());
    result["encoding"] = QJsonValue(QStringLiteral("UTF-8"));
    result["line_count"] = QJsonValue(file.size());
    result["size"] = QJsonValue(file.size());
    result["content"] = QJsonValue(QString(file.readAll()));
    file.close();

    QJsonDocument doc(result);
    content.append(doc.toJson(QJsonDocument::Compact));
}

bool ToolService::execute(const ToolModel *model, const QString &function, const QString &arguments)
{
    if (!model)
        return false;

    ToolModel::ToolEntry tool = model->toolByName(function);
    if (tool.name.isEmpty()) {
        QString msg = errorToJson(errorToJsonObject( //
            -1,
            QStringLiteral("Unable to find function: %1").arg(function)));
        emit executeCompleted(msg.toUtf8());
        return false;
    }

    qDebug() << "[ToolService] execute:" << tool.type << tool.name << arguments;

    if (tool.name == "display_project_files") {
        if (tool.option == ToolModel::AskBeforeRun) {
        }
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(arguments.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            QString msg = errorToJson(errorToJsonObject(-1, error.errorString()));
            emit executeCompleted(error.errorString().toUtf8());
            return false;
        }
        QString pathName;
        QJsonObject args = doc.object();
        if (args.contains("directory")) {
            pathName = args["directory"].toString();
        } else if (args.contains("project_path")) {
            pathName = args["project_path"].toString();
        }
        if (pathName.isEmpty()) {
            QString msg = errorToJson(errorToJsonObject( //
                -1,
                QStringLiteral("Please specify a directory name and file extensions to use to list project files.")));
            emit executeCompleted(msg.toUtf8());
            return false;
        }
        QStringList extensions;
        if (args.contains("extensions") && args["directory"].isArray()) {
            QJsonArray jext = args["directory"].toArray();
            foreach (auto item, jext) {
                if (item.isString()) {
                    extensions.append(item.toString(""));
                }
            }
        }
        QByteArray content;
        listDirectory(pathName, extensions, content);
        emit executeCompleted(content);
    } else if (tool.name == "list_source_files") {
        if (tool.option == ToolModel::AskBeforeRun) {
        }
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(arguments.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            QString msg = errorToJson(errorToJsonObject(-1, error.errorString()));
            emit executeCompleted(msg.toUtf8());
            return false;
        }
        QString pathName;
        QJsonObject args = doc.object();
        if (args.contains("directory")) {
            pathName = args["directory"].toString();
        } else if (args.contains("project_path")) {
            pathName = args["project_path"].toString();
        }
        if (pathName.isEmpty()) {
            QString msg = errorToJson(errorToJsonObject( //
                -1,
                QStringLiteral("Please specify a directory name and file extensions to use to list project files.")));
            emit executeCompleted(msg.toUtf8());
            return false;
        }
        QStringList extensions;
        if (args.contains("extensions") && args["directory"].isArray()) {
            QJsonArray jext = args["directory"].toArray();
            foreach (auto item, jext) {
                if (item.isString()) {
                    extensions.append(item.toString(""));
                }
            }
        }
        QByteArray content;
        listDirectory(pathName, extensions, content);
        emit executeCompleted(content);
        return true;
    } else if (tool.name == "read_source_file") {
        if (tool.option == ToolModel::AskBeforeRun) {
        }
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(arguments.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning() << error.errorString();
            emit executeCompleted(error.errorString().toUtf8());
            return false;
        }
        QString pathName;
        QJsonObject args = doc.object();
        if (args.contains("file")) {
            pathName = args["file"].toString();
        } else if (args.contains("file_name")) {
            pathName = args["file_name"].toString();
        } else if (args.contains("file_path")) {
            pathName = args["file_path"].toString();
        }
        if (pathName.isEmpty()) {
            QString msg = errorToJson(errorToJsonObject( //
                -1,
                QStringLiteral("Please specify a file name and file extensions to use to list project files.")));
            emit executeCompleted(msg.toUtf8());
            return false;
        }
        QByteArray content;
        readFile(pathName, content);
        emit executeCompleted(content);
        return true;
    } else if (tool.name == "write_source_file") {
        if (tool.option == ToolModel::AskBeforeRun) {
        }
        //--
        QString msg = errorToJson(errorToJsonObject( //
            -1,
            QStringLiteral("Tool call of write_source_file complete.")));
        emit executeCompleted(msg.toUtf8());
        return true;
    } else {
        qWarning() << "[ToolService] execute: (FALLBACK) Using tool object of:" << tool.name;
        QJsonDocument doc(tool.tool);
        QByteArray content = doc.toJson(QJsonDocument::Indented);
        if (!arguments.isEmpty()) {
            content.append("\r\n");
            content.append(arguments.toUtf8());
        }
        emit executeCompleted(content);
    }
    return false;
}
