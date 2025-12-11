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
        AskBeforeRun = 0,
        ToolEnabled = 1,
        ToolDisabled = 2,
    };
    Q_ENUM(ToolOption)

    enum ToolType {
        Tool = 0,
        Resource = 1,
        Prompt = 2,
    };
    Q_ENUM(ToolType)

    enum Roles {
        ToolRole = Qt::UserRole + 1,
        NameRole,
        OptionRole,
        TypeRole,
    };
    Q_ENUM(Roles)

    struct ToolEntry
    {
        QJsonObject tool;
        QString name;
        ToolOption option;
        ToolType type;
    };

    explicit ToolModel(QObject *parent = nullptr);

    // Model interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Custom methods
    void loadFromDirectory(const QFileInfo &fileInfo, ToolType type);
    QList<QJsonObject> toolObjects() const;

    void loadToolsConfig();
    bool hasExecutables() const;
    bool hasResources() const;
    bool hasPrompts() const;

signals:
    void toolAdded(const ToolModel::ToolEntry &entry);
    void toolRemoved(int index);

public slots:
    void addToolEntry(const ToolModel::ToolEntry &entry);
    void removeToolEntry(int index);

private:
    QList<ToolEntry> m_toolEntries;

private:
    inline void createConfigDir(const QDir &dir);
};
Q_DECLARE_METATYPE(ToolModel::ToolOption)
Q_DECLARE_METATYPE(ToolModel::ToolType)
Q_DECLARE_METATYPE(ToolModel::Roles)
Q_DECLARE_METATYPE(ToolModel::ToolEntry)

#endif // TOOLMODEL_H
