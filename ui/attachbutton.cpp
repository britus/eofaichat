#include "attachbutton.h"
#include <QMimeData>
#include <QUrl>

AttachButton::AttachButton(const QString &text, QWidget *parent)
    : QPushButton(parent)
{
    setAcceptDrops(true);
    setText(text);
}

void AttachButton::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void AttachButton::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        QList<QUrl> urls = event->mimeData()->urls();
        emit fileDropped(urls);
        event->acceptProposedAction();
    } else if (event->mimeData()->hasText()) {
        // Handle text drop (e.g., file paths)
        QString text = event->mimeData()->text();
        if (!text.isEmpty()) {
            // Convert text to URL and emit signal
            QUrl url(text);
            QList<QUrl> urls;
            urls.append(url);
            emit fileDropped(urls);
        }
        event->acceptProposedAction();
    }
}
