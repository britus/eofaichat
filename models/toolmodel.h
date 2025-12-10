#ifndef TOOLMODEL_H
#define TOOLMODEL_H

#include <QAbstractListModel>
#include <QDir>
#include <QJsonObject>
#include <QList>
#include <QString>

enum ToolOption { AskBeforeRun = 0, ToolEnabled = 1, ToolDisabled = 2 };
enum ToolType { Prompt, Resource, Tool };

struct ToolEntry
{
    QJsonObject tool;
    QString name;
    ToolOption option;
    ToolType type;
};

class ToolModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles { ToolRole = Qt::UserRole + 1, NameRole, OptionRole, TypeRole };

    explicit ToolModel(QObject *parent = nullptr);

    // Model interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Custom methods
    void loadFromDirectory(const QString &directory, ToolType type);
    QList<QJsonObject> toolObjects() const;

    void loadToolsConfig();
    bool hasExecutables() const;
    bool hasResources() const;
    bool hasPrompts() const;

signals:
    void toolAdded(const ToolEntry &entry);
    void toolRemoved(int index);

public slots:
    void addToolEntry(const ToolEntry &entry);
    void removeToolEntry(int index);

private:
    QList<ToolEntry> m_toolEntries;

private:
    inline void createConfigDir(const QDir &dir);
};

#endif // TOOLMODEL_H
