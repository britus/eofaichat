#pragma once
#include <QAbstractListModel>
#include <QList>
#include <QObject>
#include <QString>

class ModelListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        ObjectRole,
        OwnedByRole,
        ModelEntryRole,
    };
    Q_ENUM(Roles)

    struct ModelEntry
    {
        QString id;
        QString object;
        QString ownedBy;
    };

    explicit ModelListModel(QObject *parent = nullptr);

    // Mandatory model interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Public API to modify entries
    void addEntry(const ModelEntry &entry);
    void updateEntry(int row, const ModelEntry &entry);
    void removeEntry(int row);

    inline const QList<ModelEntry> &modelList() const { return m_entries; }

signals:
    void entryAdded(int row, const ModelListModel::ModelEntry &entry);
    void entryUpdated(int row, const ModelListModel::ModelEntry &entry);
    void entryRemoved(int row, const ModelListModel::ModelEntry &entry);
    void modelsLoaded();

public:
    // Load entries from JSON array
    void loadFrom(const QJsonArray &models);
    // Load entries from JSON string
    bool loadFrom(const QString &jsonString);

private:
    QList<ModelEntry> m_entries;
};
Q_DECLARE_METATYPE(ModelListModel::ModelEntry)
Q_DECLARE_METATYPE(ModelListModel::Roles)
