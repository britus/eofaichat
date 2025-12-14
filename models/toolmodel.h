#ifndef TOOLMODEL_H
#define TOOLMODEL_H
#include <QAbstractListModel>
#include <QDir>
#include <QJsonObject>
#include <QList>
#include <QString>

class ToolModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ToolOption {
        ToolDisabled = 0,
        ToolEnabled = 1,
        AskBeforeRun = 2,
    };
    Q_ENUM(ToolOption)

    enum ToolModelType {
        ToolUnknown = 0,
        ToolFunction = 1,
        ToolResource = 2,
        ToolPrompt = 3,
    };
    Q_ENUM(ToolModelType)

    enum Roles {
        ToolRole = Qt::UserRole + 1,
        NameRole,
        OptionRole,
        TypeRole,
        DescriptionRole,
        TitleRole,
        ExecHandlerRole,
        ExecMethodRole,
    };
    Q_ENUM(Roles)

    struct ToolModelEntry
    {
        QJsonObject tool;
        QString name;
        QString title;
        QString description;
        QString execHandler;
        QString execMethod;
        ToolOption option;
        ToolModelType type;
    };

    explicit ToolModel(QObject *parent = nullptr);

    // Model interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    // Custom methods
    void loadFromDirectory(const QFileInfo &fileInfo, ToolModelType type);
    QList<QJsonObject> toolObjects() const;
    QJsonObject toolObject(const QString &name) const;
    ToolModel::ToolModelEntry toolByName(const QString &name) const;

    void loadToolsConfig();
    bool hasExecutables() const;
    bool hasResources() const;
    bool hasPrompts() const;

signals:
    void toolAdded(const ToolModel::ToolModelEntry &entry);
    void toolRemoved(int index);

public slots:
    void addToolEntry(const ToolModel::ToolModelEntry &entry);
    void removeToolEntry(int index);

private:
    QList<ToolModelEntry> m_toolEntries;

private:
    inline bool createDirectory(const QDir &dir) const;
    inline QDir configDirectory(const QString &pathName) const;
    inline bool deployResourceFiles(const QString &resourcePath, const QDir &targetDir);
    inline bool copyDirectoryRecursively(const QString &sourcePath, const QString &targetPath);
    inline void loadToolsConfig(const QString &subPath, ToolModelType type);
};
Q_DECLARE_METATYPE(ToolModel::ToolOption)
Q_DECLARE_METATYPE(ToolModel::ToolModelType)
Q_DECLARE_METATYPE(ToolModel::Roles)
Q_DECLARE_METATYPE(ToolModel::ToolModelEntry)

#endif // TOOLMODEL_H
