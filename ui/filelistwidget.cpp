#include <filelistwidget.h>
#include <filenamelabel.h>
#include <QDebug>
#include <QFileInfo>
#include <QIcon>
#include <QStyle>

FileListWidget::FileListWidget(QWidget *parent)
    : QListView(parent)
{
    // Set up the widget with a stylesheet similar to the original
    setStyleSheet(R"(
        QListView {
            background-color: #3a3a3a;
            border: 1px solid #444;
            border-radius: 8px;
            padding: 8px;
        }
    )");

    // Create model
    m_model = new QStandardItemModel(this);
    setModel(m_model);
    
    // Set view mode to icon mode
    setViewMode(QListView::IconMode);
    setSpacing(10);
    
    // Set selection mode
    setSelectionMode(QAbstractItemView::SingleSelection);
}

void FileListWidget::addFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();

    // Create a new FileNameLabel for the file
    FileNameLabel *label = new FileNameLabel(fileName, this);

    // Store the index for removal purposes
    int index = m_fileLabels.size();
    label->setIndex(index);

    // Create a standard item for the model
    QStandardItem *item = new QStandardItem(fileName);
    
    // Set icon (you might want to customize this based on file type)
    item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    
    // Add to model
    m_model->appendRow(item);

    // Store the label for later reference
    m_fileLabels.append(label);
}

void FileListWidget::removeFile(int index)
{
    if (index < 0 || index >= m_fileLabels.size()) {
        return;
    }

    // Remove from model
    m_model->removeRow(index);

    // Remove from our list
    m_fileLabels.removeAt(index);

    // Emit signal
    emit fileRemoved(index);
}

void FileListWidget::clear()
{
    // Clear the model
    m_model->clear();
    
    // Clear our list
    m_fileLabels.clear();
}

int FileListWidget::count() const
{
    return m_fileLabels.size();
}