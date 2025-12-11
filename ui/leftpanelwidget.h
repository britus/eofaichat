#pragma once
#include <chatlistmodel.h>
#include <chatpanelwidget.h>
#include <QAbstractItemModel>
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMenu>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class LeftPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LeftPanelWidget(QWidget *parent = nullptr);

    // Method to create initial chat
    void createInitialChat(const QString &name);

signals:
    void downloadClicked();
    void aboutClicked();
    void chatSelected(QWidget *chatWidget);
    void chatRemoved(QWidget *chatWidget);

private slots:
    void onNewChat();
    void onChatItemClicked(const QModelIndex &index);
    void onEditChat();
    void onDeleteChat();
    void onConfirmDelete();
    void onCancelDelete();
    void onUpdates();
    void onChatNameChanged(const QString &newName);
    void onDeleteChatRequested(int row);
    void onAddChat();
    void onContextMenu(const QPoint &point);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    ChatListModel *chatModel;
    QListView *chatList; // Changed from QListWidget to QListView
    QPushButton *newChatButton;
    QPushButton *updatesButton;
    QPushButton *downloadsButton;
    QPushButton *aboutButton;
    QWidget *trashConfirmWidget;
    QPushButton *trashYes;
    QPushButton *trashNo;
    QTimer *autoHideTimer;
    int pendingDeleteIndex = -1;
    QMenu *contextMenu;
};
