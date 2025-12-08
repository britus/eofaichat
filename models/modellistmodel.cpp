#include "modellistmodel.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

ModelListModel::ModelListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int ModelListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant ModelListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        if (role == Qt::UserRole && !m_entries.isEmpty()) {
            return QVariant::fromValue(m_entries);
        }
        return QVariant();
    }

    const ModelEntry &entry = m_entries.at(index.row());

    switch (role) {
        case IdRole:
            return entry.id;
        case ObjectRole:
            return entry.object;
        case OwnedByRole:
            return entry.ownedBy;
        case ModelEntryRole:
            return QVariant::fromValue(entry);
        default:
            return QVariant();
    }
}

bool ModelListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return false;

    ModelEntry &entry = m_entries[index.row()];
    bool changed = false;

    switch (role) {
        case IdRole:
            if (entry.id != value.toString()) {
                entry.id = value.toString();
                changed = true;
            }
            break;
        case ObjectRole:
            if (entry.object != value.toString()) {
                entry.object = value.toString();
                changed = true;
            }
            break;
        case OwnedByRole:
            if (entry.ownedBy != value.toString()) {
                entry.ownedBy = value.toString();
                changed = true;
            }
            break;
        default:
            return false;
    }

    if (changed) {
        emit dataChanged(index, index, {role});
        emit entryUpdated(index.row(), entry);
    }
    return changed;
}

Qt::ItemFlags ModelListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

QHash<int, QByteArray> ModelListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[ObjectRole] = "object";
    roles[OwnedByRole] = "ownedBy";
    return roles;
}

void ModelListModel::addEntry(const ModelEntry &entry)
{
    int row = m_entries.size();
    beginInsertRows(QModelIndex(), row, row);
    m_entries.append(entry);
    endInsertRows();
    emit entryAdded(row, entry);
}

void ModelListModel::updateEntry(int row, const ModelEntry &entry)
{
    if (row < 0 || row >= m_entries.size())
        return;

    m_entries[row] = entry;
    QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
    emit entryUpdated(row, m_entries[row]);
}

void ModelListModel::removeEntry(int row)
{
    if (row < 0 || row >= m_entries.size())
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_entries.removeAt(row);
    endRemoveRows();
    ModelEntry removed = m_entries.at(row);
    beginRemoveRows(QModelIndex(), row, row);
    m_entries.removeAt(row);
    endRemoveRows();
    emit entryRemoved(row, removed);
}

void ModelListModel::loadFrom(const QJsonArray &models)
{
    beginResetModel();
    m_entries.clear();

    for (const auto &value : models) {
        if (!value.isObject())
            continue;

        QJsonObject obj = value.toObject();
        ModelEntry entry;
        entry.id = obj.value("id").toString();
        entry.object = obj.value("object").toString();
        entry.ownedBy = obj.value("owned_by").toString();

        m_entries.append(entry);
    }

    endResetModel();
    emit modelsLoaded();
}

bool ModelListModel::loadFrom(const QString &jsonString)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError || !doc.isArray())
        return false;

    loadFrom(doc.array());
    return true;
}
