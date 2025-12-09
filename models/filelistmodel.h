#pragma once
#include <QAbstractListModel>
#include <QStringList>
#include <QObject>

class FileListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit FileListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // Add file to the list
    void addFile(const QString &filePath);
    
    // Remove file from the list
    void removeFile(int index);
    
    // Get file path at index
    QString filePath(int index) const;
    
    // Get file name (without path) at index
    QString fileName(int index) const;

signals:
    void added(const QString &filePath);
    void edited(int index, const QString &filePath);
    void removed(int index);

private:
    QStringList m_files;
};