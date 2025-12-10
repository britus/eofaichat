#pragma once
#include <filelistmodel.h>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QList>
#include <QListView>
#include <QPixmap>
#include <QString>
#include <QContextMenuEvent>
#include <QMenu>

class FileListWidget : public QListView
{
    Q_OBJECT

public:
    explicit FileListWidget(FileListModel *model, QWidget *parent = nullptr);
    int count() const;

public slots:
    void clear();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onFileAdded(int index, FileItem *item);
    void onFileRemoved(int index);
    void deleteSelectedFile();

signals:
    void fileRemoved(int index);

private:
    FileListModel *m_model;
    QMenu *m_contextMenu;
};