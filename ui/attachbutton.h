#pragma once
#include <QPushButton>
#include <QDragEnterEvent>
#include <QDropEvent>

class AttachButton : public QPushButton
{
    Q_OBJECT

public:
    explicit AttachButton(const QString& text, QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

signals:
    void fileDropped(const QList<QUrl> &urls);
};
