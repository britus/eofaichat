#include "filelistmodel.h"
#include <QFileInfo>

FileListModel::FileListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FileListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_files.count();
}

QVariant FileListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_files.count())
        return QVariant();

    if (role == Qt::DisplayRole) {
        QFileInfo fileInfo(m_files[index.row()]);
        return fileInfo.fileName();
    }

    return QVariant();
}

bool FileListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_files.count())
        return false;

    if (role == Qt::EditRole) {
        QString newFilePath = value.toString();
        m_files[index.row()] = newFilePath;
        emit edited(index.row(), newFilePath);
        return true;
    }

    return false;
}

void FileListModel::addFile(const QString &filePath)
{
    beginInsertRows(QModelIndex(), m_files.count(), m_files.count());
    m_files.append(filePath);
    endInsertRows();
    
    emit added(filePath);
}

void FileListModel::removeFile(int index)
{
    if (index < 0 || index >= m_files.count())
        return;
        
    beginRemoveRows(QModelIndex(), index, index);
    QString removedFilePath = m_files.takeAt(index);
    endRemoveRows();
    
    emit removed(index);
}

QString FileListModel::filePath(int index) const
{
    if (index < 0 || index >= m_files.count())
        return QString();
    return m_files[index];
}

QString FileListModel::fileName(int index) const
{
    if (index < 0 || index >= m_files.count())
        return QString();
    QFileInfo fileInfo(m_files[index]);
    return fileInfo.fileName();
}