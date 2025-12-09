#pragma once
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QList>
#include <QString>

class FileNameLabel : public QWidget
{
    Q_OBJECT

public:
    explicit FileNameLabel(const QString &fileName, QWidget *parent = nullptr);

    QString fileName() const;
    void setFileName(const QString &fileName);

signals:
    void removeRequested(int index);

private slots:
    void onRemoveClicked();

private:
    QLabel *m_label;
    QToolButton *m_removeButton;
    int m_index;
};

class FileListWidget : public QWidget
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
    QGridLayout *m_layout;
    QList<FileNameLabel*> m_fileLabels;
};