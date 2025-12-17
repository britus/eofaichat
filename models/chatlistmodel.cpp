#include <chatpanelwidget.h>
#include <leftpanelwidget.h>
#include <mainwindow.h>
#include <QApplication>
#include <QInputDialog>
#include <QLabel>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStyle>

ChatListModel::ChatListModel(QObject *parent)
    : QAbstractItemModel(parent)
{}

QModelIndex ChatListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= m_chats.size() || column != 0)
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex ChatListModel::parent(const QModelIndex & /*child*/) const
{
    return QModelIndex();
}

int ChatListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_chats.size();
}

int ChatListModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 1;
}

QVariant ChatListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_chats.size())
        return QVariant();

    const ChatData &chat = m_chats[index.row()];

    if (role == Qt::DisplayRole)
        return chat.name;
    else if (role == Qt::UserRole)
        return QVariant::fromValue(chat.widget);

    return QVariant();
}

bool ChatListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_chats.size())
        return false;

    if (role == Qt::EditRole) {
        m_chats[index.row()].name = value.toString();
        emit dataChanged(index, index, {Qt::DisplayRole});
        return true;
    }

    return false;
}

Qt::ItemFlags ChatListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

void ChatListModel::addChat(const QString &name, ChatPanelWidget *widget)
{
    beginInsertRows(QModelIndex(), m_chats.size(), m_chats.size());
    m_chats.append(ChatData(name, widget));
    endInsertRows();
    emit chatWidgetAdded(widget);
}

void ChatListModel::removeChat(int row)
{
    if (row < 0 || row >= m_chats.size())
        return;

    QWidget *widget = m_chats[row].widget;

    beginRemoveRows(QModelIndex(), row, row);
    m_chats.removeAt(row);
    endRemoveRows();

    emit chatWidgetRemoved(widget);
}

void ChatListModel::renameChat(int row, const QString &newName)
{
    if (row < 0 || row >= m_chats.size())
        return;

    m_chats[row].name = newName;
    emit dataChanged(index(row, 0), index(row, 0), {Qt::DisplayRole});
}

ChatListModel::ChatData *ChatListModel::chatData(int row) const
{
    if (row < 0 || row >= m_chats.size())
        return nullptr;
    return const_cast<ChatData *>(&m_chats[row]);
}

int ChatListModel::chatCount() const
{
    return m_chats.size();
}

// New method to get ChatModel from a ChatPanelWidget
ChatModel *ChatListModel::chatModel(int row) const
{
    if (row < 0 || row >= m_chats.size())
        return nullptr;
    // Access the ChatPanelWidget and get its ChatModel
    ChatPanelWidget *panelWidget = m_chats[row].widget;
    if (panelWidget) {
        // Assuming ChatPanelWidget has a method to get its ChatModel
        // This is a bit of a hack, but it's the only way to get the model from the widget
        // We'll use a direct cast, which is safe as long as we know the type.
        return panelWidget->chatModel();
    }
    return nullptr;
}
