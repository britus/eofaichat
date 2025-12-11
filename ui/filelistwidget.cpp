#include <filelistwidget.h>
#include <filenamelabel.h>
#include <QAction>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QMessageBox>
#include <QMimeData>
#include <QResizeEvent>
#include <QStyle>
#include <QUrl>

FileListWidget::FileListWidget(FileListModel *model, QWidget *parent)
    : QListView(parent)
    , m_model(model)
{
    // Create model
    setModel(m_model);

    // Connect file list model signals
    connect(m_model, &FileListModel::added, this, &FileListWidget::onFileAdded);
    connect(m_model, &FileListModel::removed, this, &FileListWidget::onFileRemoved);

    // Setup visual style
    setSpacing(12);
    setViewMode(QListView::IconMode);
    setItemAlignment(Qt::AlignLeft);
    setFlow(QListView::LeftToRight);
    setResizeMode(QListView::Adjust);
    setLayoutMode(QListView::Batched);
    setWrapping(true);

    // Set selection mode (delete multiple attachments)
    setSelectionMode(QAbstractItemView::SingleSelection);

    // Enable drag and drop
    setAcceptDrops(true);
    setDragEnabled(true);
    setDropIndicatorShown(true);

    // Create context menu
    m_contextMenu = new QMenu(this);
    QAction *addFileAction = m_contextMenu->addAction("Add File");
    QAction *deleteFileAction = m_contextMenu->addAction("Delete File");

    connect(addFileAction, &QAction::triggered, this, [this]() {
        QStringList files = QFileDialog::getOpenFileNames(this, "Select Files", QDir::homePath());
        for (const QString &file : files) {
            QFileInfo fileInfo(file);
            if (fileInfo.exists()) {
                QIcon icon = style()->standardIcon(QStyle::SP_FileIcon);
                m_model->addFile(file, icon);
            }
        }
    });

    connect(deleteFileAction, &QAction::triggered, this, &FileListWidget::deleteSelectedFile);
}

// ---------------- File List Model Events ----------------
void FileListWidget::onFileAdded(int /*index*/, FileItem * /*item*/)
{
    setVisible(m_model->rowCount() > 0);
}

void FileListWidget::onFileRemoved(int index)
{
    setVisible(m_model->rowCount() > 0);
    emit fileRemoved(index);
}

void FileListWidget::clear()
{
    m_model->clear();
}

int FileListWidget::count() const
{
    return m_model->rowCount();
}

// ---------------- Key Press Event ----------------
void FileListWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
        deleteSelectedFile();
    } else if (event->key() == Qt::Key_Backspace) {
        deleteSelectedFile();
    } else {
        QListView::keyPressEvent(event);
    }
}

// ---------------- Context Menu Event ----------------
void FileListWidget::contextMenuEvent(QContextMenuEvent *event)
{
    m_contextMenu->popup(event->globalPos());
}

// ---------------- Delete Selected File ----------------
void FileListWidget::deleteSelectedFile()
{
    QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
    if (!selectedIndexes.isEmpty()) {
        // Get the first selected index and remove that file
        int row = selectedIndexes.first().row();
        m_model->removeFile(row);
    }
}

// ---------------- Drag and Drop Events ----------------
void FileListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept drag events if they contain URLs (files)
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void FileListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void FileListWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl &url : urls) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();
                QFileInfo fileInfo(filePath);
                if (fileInfo.exists()) {
                    QIcon icon = style()->standardIcon(QStyle::SP_FileIcon);
                    m_model->addFile(filePath, icon);
                }
            }
        }
    } else if (event->mimeData()->hasText()) {
        QString text = event->mimeData()->text();
        if (!text.isEmpty()) {
            QFileInfo fileInfo(text);
            if (fileInfo.exists()) {
                QIcon icon = style()->standardIcon(QStyle::SP_FileIcon);
                m_model->addFile(text, icon);
            }
        }
    }

    event->acceptProposedAction();
}

// ---------------- Resize Event ----------------
void FileListWidget::resizeEvent(QResizeEvent *event)
{
    // Call the parent resize event
    QListView::resizeEvent(event);

    // Ensure proper layout after resize
    setWrapping(true);
}
