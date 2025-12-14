#include <filelistmodel.h>
#include <QDir>
#include <QFileInfo>

FileListModel::FileListModel(QObject *parent)
    : QStandardItemModel(parent)
{
    //-
}

void FileListModel::addFile(const QString &filePath, const QIcon &icon)
{
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    FileItem *item = new FileItem(fileInfo, icon, fileName);
    int count = rowCount();
    beginInsertRows(QModelIndex(), count, count);
    QStandardItemModel::appendRow(item);
    endInsertRows();

    emit added(count, item);
}

void FileListModel::removeFile(int index)
{
    if (index < 0 || index >= rowCount())
        return;

    beginRemoveRows(QModelIndex(), index, index);
    QStandardItemModel::removeRows(index, 1);
    endRemoveRows();

    emit removed(index);
}

QString FileListModel::filePath(int index) const
{
    if (index < 0 || index >= rowCount())
        return QString();
    if (FileItem *_item = dynamic_cast<FileItem *>(item(index))) {
        QFileInfo fileInfo(_item->fileInfo());
        return fileInfo.absoluteFilePath();
    }
    return QString();
}

QString FileListModel::fileName(int index) const
{
    if (index < 0 || index >= rowCount())
        return QString();
    if (FileItem *_item = dynamic_cast<FileItem *>(item(index))) {
        QFileInfo fileInfo(_item->fileInfo());
        return fileInfo.fileName();
    }
    return QString();
}

QByteArray FileListModel::readFileContent(int index) const
{
    QByteArray result;
    if (index > -1 && index < rowCount()) {
        if (FileItem *_item = dynamic_cast<FileItem *>(item(index))) {
            QFileInfo fileInfo(_item->fileInfo());
            QFile file(fileInfo.absoluteFilePath());
            if (file.open(QFile::ReadOnly)) {
                result = file.readAll();
                file.close();
            }
        }
    }
    return result;
}

void FileListModel::loadFilesContent(QByteArray &content)
{
    for (int i = 0; i < rowCount(); i++) {
        content.append(readFileContent(i));
    }
}

void FileListModel::clear()
{
    QStandardItemModel::clear();
    emit removed(-1);
}
