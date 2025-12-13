#ifndef LLMCONNECTIONMODEL_H
#define LLMCONNECTIONMODEL_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QAbstractTableModel>

class LLMConnectionModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit LLMConnectionModel(QObject *parent = nullptr);
    ~LLMConnectionModel();

    // Connection data structure
    struct ConnectionData
    {
        QString name;
        QString provider;
        QString apiUrl;
        QString apiKey;
        bool isDefault;
        bool isEnabled;
    };

    enum Column
    {
        NameColumn = 0,
        ProviderColumn,
        ApiUrlColumn,
        IsDefaultColumn,
        IsEnabledColumn,
        ColumnCount
    };

    // Methods to manage connections
    void loadConnections();
    void saveConnections();
    QList<ConnectionData> getAllConnections() const;
    ConnectionData getConnection(const QString &name) const;
    void addConnection(const ConnectionData &connection);
    void updateConnection(const QString &name, const ConnectionData &connection);
    void removeConnection(const QString &name);
    void setDefaultConnection(const QString &name);
    QString getDefaultConnectionName() const;

    // QAbstractTableModel methods
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

private:
    QString getConfigFileName() const;
    void loadDefaultConnections();

private:
    QMap<QString, ConnectionData> m_connections;
    QString m_defaultConnectionName;
};

#endif // LLMCONNECTIONMODEL_H