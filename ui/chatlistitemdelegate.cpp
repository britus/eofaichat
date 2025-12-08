#include <chatlistitemdelegate.h>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>

ChatListItemDelegate::ChatListItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    //
}

void ChatListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // First call the parent paint to draw the item normally
    QStyledItemDelegate::paint(painter, option, index);

    // Draw delete button on hover - make sure it's painted on top
    if (option.state & QStyle::State_MouseOver) {
        QRect deleteButtonRect = option.rect;
        deleteButtonRect.setLeft(deleteButtonRect.right() - 30);
        deleteButtonRect.setWidth(25);
        deleteButtonRect.setTop(deleteButtonRect.top() + 5);
        deleteButtonRect.setHeight(20);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // Draw the delete button with a more visible appearance
        painter->setBrush(QColor(255, 0, 0, 180)); // Semi-transparent red
        painter->setPen(QPen(Qt::white, 1));
        painter->drawRect(deleteButtonRect);

        // Draw X symbol
        painter->setPen(Qt::white);
        painter->drawLine(deleteButtonRect.left() + 4, //
                          deleteButtonRect.top() + 5,
                          deleteButtonRect.right() - 4,
                          deleteButtonRect.bottom() - 5);
        painter->drawLine(deleteButtonRect.left() + 4, //
                          deleteButtonRect.bottom() - 5,
                          deleteButtonRect.right() - 4,
                          deleteButtonRect.top() + 5);
        painter->restore();
    }
}

QSize ChatListItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
{
    return QSize(option.rect.width(), 40);
}

bool ChatListItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QRect deleteButtonRect = option.rect;
        deleteButtonRect.setLeft(deleteButtonRect.right() - 30);
        deleteButtonRect.setWidth(25);
        deleteButtonRect.setTop(deleteButtonRect.top() + 5);
        deleteButtonRect.setHeight(20);

        if (deleteButtonRect.contains(mouseEvent->pos())) {
            // Emit signal to delete chat
            emit deleteRequested(index.row());
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
