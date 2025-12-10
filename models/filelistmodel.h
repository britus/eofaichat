#pragma once
#include <QAbstractListModel>
#include <QFileInfo>
#include <QObject>
#include <QStandardItemModel>
#include <QStringList>
#include <QStyle>

class FileItem : public QStandardItem
{
public:
    FileItem(const QFileInfo &info, const QIcon &icon, const QString &text)
        : QStandardItem(icon, text)
        , m_info(info) {};
    explicit FileItem(const QString &text)
        : QStandardItem(text) {};
    FileItem(const QIcon &icon, const QString &text)
        : QStandardItem(icon, text) {};
    explicit FileItem(int rows, int columns = 1)
        : QStandardItem(rows, columns) {};
    inline void setFileInfo(const QFileInfo &info) { m_info = info; }
    inline const QFileInfo &fileInfo() const { return m_info; }

private:
    QFileInfo m_info;
};
Q_DECLARE_METATYPE(FileItem)

class FileListModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit FileListModel(QObject *parent = nullptr);

    // Add file to the list
    void addFile(const QString &filePath, const QIcon &icon);

    // Remove file from the list
    void removeFile(int index);

    // Get file path at index
    QString filePath(int index) const;

    // Get file name (without path) at index
    QString fileName(int index) const;

    void loadContentOfFiles(QByteArray &content);
    void clear();

signals:
    void added(int index, FileItem *item);
    void edited(int index, FileItem *item);
    void removed(int index);
};
