#pragma once
#include <QListView>
#include <QStandardItemModel>
#include <QString>
#include <QList>

class FileNameLabel;

class FileListWidget : public QListView
{
    Q_OBJECT

public:
    explicit FileListWidget(QWidget *parent = nullptr);

    void addFile(const QString &filePath);
    void removeFile(int index);
    int count() const;

public slots:
    void clear();

signals:
    void fileRemoved(int index);

private:
    QStandardItemModel *m_model;
    QList<FileNameLabel *> m_fileLabels;
};