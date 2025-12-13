#ifndef LLMCONNECTIONSDIALOG_H
#define LLMCONNECTIONSDIALOG_H
#include <llmconnectionmodel.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

class LLMConnectionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LLMConnectionsDialog(LLMConnectionModel *model, QWidget *parent = nullptr);
    ~LLMConnectionsDialog();

private slots:
    void onFinished(QNetworkReply *reply);
    void onError(QNetworkReply::NetworkError error);
    void onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void onAddClicked();
    void onEditClicked();
    void onRemoveClicked();
    void onTestClicked();
    void onSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void onModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
    void onConnectionAdded(const QModelIndex &parent, int first, int last);
    void onDialogAccepted();
    void onDialogRejected();

private:
    inline void setupUI();
    inline void setupConnections();
    inline void loadConnections();
    inline void updateButtons();
    inline void clearForm();
    inline void populateForm(const LLMConnectionModel::ConnectionData &connection);
    inline LLMConnectionModel::ConnectionData getFormData() const;
    inline bool validateForm() const;
    inline bool validateEditing() const;
    inline void showTestResult(const QString &result);

private:
    LLMConnectionModel *m_model;
    QNetworkAccessManager *m_networkManager;
    QTableView *m_tableView;
    QLineEdit *m_nameEdit;
    QComboBox *m_providerCombo;
    QLineEdit *m_apiUrlEdit;
    QLineEdit *m_apiKeyEdit;
    QCheckBox *m_isEnabledCheck;
    QCheckBox *m_isDefaultCheck;
    QPushButton *m_addButton;
    QPushButton *m_updateButton;
    QPushButton *m_removeButton;
    QPushButton *m_testButton;
    QPushButton *m_saveButton;
    QPushButton *m_cancelButton;
    QHBoxLayout *m_buttonLayout;
    QWidget *m_formWidget;
    QLabel *m_statusLabel;
    bool m_isAdding;
};

#endif // LLMCONNECTIONSDIALOG_H
