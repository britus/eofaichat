#include <filenamelabel.h>
#include <QIcon>
#include <QStyle>

FileNameLabel::FileNameLabel(const QString &fileName, QWidget *parent)
    : QWidget(parent)
    , m_index(0)
{
    m_label = new QLabel(fileName, this);
    m_removeButton = new QToolButton(this);

    // Set up remove button
    m_removeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    m_removeButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    m_removeButton->setAutoRaise(true);

    // Set up layout
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_label);
    layout->addWidget(m_removeButton);
    layout->setAlignment(Qt::AlignLeft);

    // Connect signal
    connect(m_removeButton, &QToolButton::clicked, this, &FileNameLabel::onRemoveClicked);
}

QString FileNameLabel::fileName() const
{
    return m_label->text();
}

void FileNameLabel::setFileName(const QString &fileName)
{
    m_label->setText(fileName);
}

void FileNameLabel::setIndex(int index)
{
    m_index = index;
}

void FileNameLabel::onRemoveClicked()
{
    emit removeRequested(m_index);
}
