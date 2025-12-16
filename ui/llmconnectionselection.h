#ifndef LLMCONNECTIONSELECTION_H
#define LLMCONNECTIONSELECTION_H

#include <QDialog>
#include <QListView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QItemSelection>

class LLMConnectionModel;

class LLMConnectionSelection : public QDialog
{
    Q_OBJECT

public:
    explicit LLMConnectionSelection(LLMConnectionModel *model, QWidget *parent = nullptr);
    ~LLMConnectionSelection();

    QString selectedConnectionName() const;

private slots:
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onOkClicked();
    void onCancelClicked();
    void onItemDoubleClicked(const QModelIndex &index);

private:
    void setupUI();
    void loadConnections();

private:
    LLMConnectionModel *m_model;
    QListView *m_listView;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
    QLabel *m_statusLabel;
    QString m_selectedConnectionName;
};

#endif // LLMCONNECTIONSELECTION_H