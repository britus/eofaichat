#pragma once
#include <QAbstractItemModel>
#include <QHBoxLayout>
#include <QListWidget>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class ChatPanelWidget;
class ChatListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    struct ChatData
    {
        QString name;
        ChatPanelWidget *widget;

        ChatData(const QString &n, ChatPanelWidget *w)
            : name(n)
            , widget(w)
        {}
    };

    explicit ChatListModel(QObject *parent = nullptr);

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Custom methods
    void addChat(const QString &name, ChatPanelWidget *widget);
    void removeChat(int row);
    void renameChat(int row, const QString &newName);
    ChatData *getChatData(int row) const;
    int chatCount() const;

signals:
    void chatWidgetRemoved(QWidget *widget);
    void chatWidgetAdded(QWidget *widget);

private:
    QList<ChatData> m_chats;
};
