#pragma once
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QWidget>

class FileNameLabel : public QWidget
{
    Q_OBJECT

public:
    explicit FileNameLabel(const QString &fileName, QWidget *parent = nullptr);

    QString fileName() const;
    void setFileName(const QString &fileName);
    void setIndex(int index);

signals:
    void removeRequested(int index);

private slots:
    void onRemoveClicked();

private:
    QLabel *m_label;
    QToolButton *m_removeButton;
    int m_index;
};
