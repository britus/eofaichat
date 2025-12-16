#pragma once
#include <chatlistmodel.h>
#include <chatpanelwidget.h>
#include <llmconnectionmodel.h>
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

signals:
    void downloadClicked();
    void aboutClicked();
    void chatSelected(QWidget *chatWidget);
    void chatRemoved(QWidget *chatWidget);
    void connectionSelected(LLMConnection *connection);

public slots:
    void onNewChat();

private slots:
    void onChatItemClicked(const QModelIndex &index);
    void onEditChat();
    void onDeleteChat();
    void onConfirmDelete();
    void onCancelDelete();
    void onSelectConnection();
    void onChatNameChanged(const QString &newName);
    void onDeleteChatRequested(int row);
    void onContextMenu(const QPoint &point);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    ChatListModel *m_chatListModel;
    LLMConnectionModel *m_llmModel;
    QListView *m_chatList; // Changed from QListWidget to QListView
    QPushButton *m_newChatButton;
    QPushButton *m_updatesButton;
    //QPushButton *m_downloadsButton;
    QPushButton *m_aboutButton;
    QWidget *m_trashConfirmWidget;
    QPushButton *m_trashYes;
    QPushButton *m_trashNo;
    QTimer *m_autoHideTimer;
    int m_pendingDeleteIndex = -1;
    QMenu *m_contextMenu;
    LLMConnection m_connection;
};
