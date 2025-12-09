#pragma once
#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>

class FileNameLabel : public QWidget
{
    Q_OBJECT

public:
    explicit FileNameLabel(const QString &fileName, QWidget *parent = nullptr);

    QString fileName() const;
    void setFileName(const QString &fileName);

signals:
    void removeRequested(int index);

private slots:
    void onRemoveClicked();

private:
    QLabel *m_label;
    QToolButton *m_removeButton;
    int m_index;
};