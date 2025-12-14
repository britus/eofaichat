#pragma once
#include <toolmodel.h>
#include <QObject>

class ToolService : public QObject
{
    Q_OBJECT

public:
    explicit ToolService(QObject *parent = nullptr);

public slots:
    /**
     * @brief Displays all source code files in the project
     * @param projectPath Path name
     * @param recursive true/false
     * @param sort_by sorting
     * @param extensions File extension filter
     * @return JSON object with file list and summary
     */
    Q_INVOKABLE QJsonObject displayProjectFiles(const QString &projectPath, bool recursive = true, const QString &sort_by = "name", const QStringList extensions = {}) const;

    /**
     * @brief Lists all source code files in the project directory
     * @param projectPath file path
     * @param extensions File extension filter
     * @return JSON object with results
     */
    Q_INVOKABLE QJsonObject listSourceFiles(const QString &projectPath, const QStringList &extensions = {}) const;

    /**
     * @brief Reads the contents of a source code file
     * @param filePath file path
     * @param length length to read
     * @param offset read starting at offset
     * @return JSON object with file content
     */
    Q_INVOKABLE QJsonObject readSourceFile(const QString &filePath, qsizetype length = -1, qsizetype offset = -1) const;

    /**
     * @brief Saves changes to a source code file
     * @param filePath file path
     * @param content new content
     * @param create_backup true or false
     * @return JSON object with result status
     */
    Q_INVOKABLE QJsonObject writeSourceFile(const QString &filePath, const QByteArray &content, bool create_backup = true) const;

    /**
     * @brief execute
     * @param tool
     * @param arguments
     * @return
     */
    Q_INVOKABLE QJsonObject execute(const ToolModel::ToolModelEntry &tool, const QString &arguments) const;
    /**
     * @brief createErrorResponse
     * @param strErrorMsg
     * @return
     */
    Q_INVOKABLE QJsonObject createErrorResponse(const QString &strErrorMsg) const;
    /**
     * @brief getFileExtensions
     * @param jsonArray
     * @return
     */
    Q_INVOKABLE QStringList getFileExtensions(const QJsonArray &jsonArray) const;
    /**
     * @brief fileInfoToJson
     * @param fileInfo
     * @param strBaseDir
     * @return
     */
    Q_INVOKABLE QJsonObject fileInfoToJson(const QFileInfo &fileInfo, const QString &strBaseDir) const;

private:
    typedef std::function<const QJsonObject(const ToolModel::ToolModelEntry &, const QJsonObject &)> ToolFunctionType;
    // Standard file extensions for source code
    QStringList DEFAULT_EXTENSIONS = QStringList() //
                                     << ".cpp"     //
                                     << ".h"       //
                                     << ".hpp"     //
                                     << ".c"       //
                                     << ".cc"      //
                                     << ".cxx"     //
                                     << ".hxx"     //
                                     << ".java";
    QMap<QString, ToolFunctionType> m_functions;

private:
    QJsonObject listDirectory(const ToolModel::ToolModelEntry &tool, const QJsonObject &args) const;
    QString createBackupPath(const QString &strOriginalPath) const;
    bool isValidPath(const QString &strPath) const;
    QList<QFileInfo> findSourceFiles(const QString &strPath, const QStringList &strExtensions, bool bRecursive) const;
};
