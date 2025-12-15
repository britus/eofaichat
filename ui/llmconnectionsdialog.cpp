#include <llmchatclient.h>
#include <llmconnectionsdialog.h>
#include <QApplication>
#include <QDebug>
#include <QGroupBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

LLMConnectionsDialog::LLMConnectionsDialog(LLMConnectionModel *model, QWidget *parent)
    : QDialog(parent)
    , m_model(model)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_isAdding(false)
{
    setupUI();
    setupConnections();
    loadConnections();
    updateButtons();
}

LLMConnectionsDialog::~LLMConnectionsDialog() {}

void LLMConnectionsDialog::setupUI()
{
    setWindowTitle(tr("%1 - LLM Connection Manager").arg(qApp->applicationDisplayName()));
    resize(800, 600);

    setStyleSheet(qApp->styleSheet());

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // Table view for connections
    m_tableView = new QTableView(this);
    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Remove vertical header
    m_tableView->verticalHeader()->setVisible(false);
    // Remove borders around horizontal header columns
    m_tableView->horizontalHeader()->setStyleSheet("QHeaderView::section { border: none; }");

    // Form layout for connection details
    m_formWidget = new QWidget(this);
    m_formWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QGridLayout *formLayout = new QGridLayout();
    m_formWidget->setLayout(formLayout);
    m_formWidget->setContentsMargins(12, 12, 12, 12);
    m_formWidget->setEnabled(false);

    // Left column - form fields
    m_nameEdit = new QLineEdit(m_formWidget);
    m_nameEdit->setPlaceholderText(tr("Connection name"));
    formLayout->addWidget(new QLabel(tr("Name:"), m_formWidget), 0, 0);
    formLayout->addWidget(m_nameEdit, 0, 1);

    m_providerCombo = new QComboBox(m_formWidget);
    m_providerCombo->addItem(tr("LLM-Studio"));
    m_providerCombo->addItem(tr("llamacpp"));
    m_providerCombo->addItem(tr("OpenAI"));
    m_providerCombo->addItem(tr("Anthropic"));
    m_providerCombo->addItem(tr("Hugging Face"));
    m_providerCombo->addItem(tr("Custom"));
    formLayout->addWidget(new QLabel(tr("Provider:"), m_formWidget), 1, 0);
    formLayout->addWidget(m_providerCombo, 1, 1);

    m_apiUrlEdit = new QLineEdit(m_formWidget);
    m_apiUrlEdit->setPlaceholderText(tr("API URL"));
    formLayout->addWidget(new QLabel(tr("API URL:"), m_formWidget), 2, 0);
    formLayout->addWidget(m_apiUrlEdit, 2, 1);

    m_apiKeyEdit = new QLineEdit(m_formWidget);
    m_apiKeyEdit->setPlaceholderText(tr("API Key (optional)"));
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    formLayout->addWidget(new QLabel(tr("API Key:"), m_formWidget), 3, 0);
    formLayout->addWidget(m_apiKeyEdit, 3, 1);

    m_authTypeCombo = new QComboBox(m_formWidget);
    m_authTypeCombo->addItem(tr("Token"));
    m_authTypeCombo->addItem(tr("Bearer"));
    formLayout->addWidget(new QLabel(tr("Auth.Type:"), m_formWidget), 4, 0);
    formLayout->addWidget(m_authTypeCombo, 4, 1);

    QGroupBox *optionBox = new QGroupBox(m_formWidget);
    optionBox->setTitle(tr("Options"));

    QGridLayout *optionLayout = new QGridLayout(optionBox);
    optionLayout->setContentsMargins(12, 12, 12, 12);
    optionBox->setLayout(optionLayout);

    m_isEnabledCheck = new QCheckBox(tr("Enabled"), optionBox);
    m_isEnabledCheck->setChecked(true);
    m_isDefaultCheck = new QCheckBox(tr("Default"), optionBox);
    m_isDefaultCheck->setChecked(true);
    optionLayout->addWidget(m_isEnabledCheck, 0, 0);
    optionLayout->addWidget(m_isDefaultCheck, 1, 0);
    optionLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 2, 0);

    // Move optionBox to top/right corner and expand vertically over all 4 rows
    formLayout->addWidget(optionBox, 0, 2, 4, 1); // (row, column, rowSpan, columnSpan)

    // Create a container for the action buttons
    QWidget *buttonContainer = new QWidget(this);
    buttonContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    m_addButton = new QPushButton(tr("Add"), this);
    m_removeButton = new QPushButton(tr("Remove"), this);
    m_applyButton = new QPushButton(tr("Apply"), this);
    m_testButton = new QPushButton(tr("Test"), this);
    m_cancelButton = new QPushButton(tr("Close"), this);

    // Add buttons to container with spacing
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_removeButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_testButton);
    buttonLayout->addWidget(m_cancelButton);

    // Status label
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("QLabel { color: blue; max-height: 20px;}");

    // Add widgets to main layout
    mainLayout->addWidget(m_tableView);
    mainLayout->addWidget(m_formWidget);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(buttonContainer);

    // Set initial state
    clearForm();
}

void LLMConnectionsDialog::setupConnections()
{
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &LLMConnectionsDialog::onFinished);
    connect(m_networkManager, &QNetworkAccessManager::sslErrors, this, &LLMConnectionsDialog::onSslErrors);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::currentChanged, this, &LLMConnectionsDialog::onSelectionChanged);
    connect(m_model, &QAbstractItemModel::dataChanged, this, &LLMConnectionsDialog::onModelDataChanged);
    connect(m_model, &QAbstractItemModel::rowsInserted, this, &LLMConnectionsDialog::onConnectionAdded);

    connect(m_nameEdit, &QLineEdit::editingFinished, this, [this]() { //
        m_applyButton->setEnabled(validateEditing());
        m_testButton->setEnabled(m_applyButton->isEnabled());
    });
    connect(m_apiUrlEdit, &QLineEdit::editingFinished, this, [this]() { //
        m_applyButton->setEnabled(validateEditing());
        m_testButton->setEnabled(m_applyButton->isEnabled());
    });

    connect(m_addButton, &QPushButton::clicked, this, &LLMConnectionsDialog::onAddClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &LLMConnectionsDialog::onEditClicked);
    connect(m_removeButton, &QPushButton::clicked, this, &LLMConnectionsDialog::onRemoveClicked);
    connect(m_testButton, &QPushButton::clicked, this, &LLMConnectionsDialog::onTestClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &LLMConnectionsDialog::onDialogRejected);
}

void LLMConnectionsDialog::loadConnections()
{
    // The model is already loaded, we just need to refresh the view
    m_tableView->resizeColumnsToContents();
    m_tableView->selectRow(m_model->rowCount() > 0 ? 0 : -1);
}

void LLMConnectionsDialog::updateButtons()
{
    bool hasSelection = m_tableView->currentIndex().isValid() && m_model->rowCount() > 0;
    //bool hasSelection = m_tableView->selectionModel()->hasSelection();
    m_applyButton->setEnabled(hasSelection);
    m_removeButton->setEnabled(hasSelection);
    m_testButton->setEnabled(hasSelection);
    m_formWidget->setEnabled(hasSelection || m_isAdding);
}

void LLMConnectionsDialog::clearForm()
{
    m_nameEdit->clear();
    m_apiUrlEdit->clear();
    m_apiKeyEdit->clear();
    m_isEnabledCheck->setChecked(false);
    m_isDefaultCheck->setChecked(false);
    m_providerCombo->setCurrentIndex(m_providerCombo->count() - 1); // set custom
    m_providerCombo->setCurrentIndex(0);
    // Disable editing buttons when form is cleared
    m_applyButton->setEnabled(false);
    m_removeButton->setEnabled(false);
    m_testButton->setEnabled(false);
}

void LLMConnectionsDialog::populateForm(const LLMConnection &connection)
{
    m_nameEdit->setText(connection.name());
    // Find and set m_provider
    int index = m_providerCombo->findText(connection.provider());
    if (index >= 0) {
        m_providerCombo->setCurrentIndex(index);
    } else {
        m_providerCombo->setCurrentText(connection.provider());
    }
    switch (connection.authType()) {
        case LLMConnection::AuthType::AuthToken: {
            m_authTypeCombo->setCurrentIndex(0);
            break;
        }
        case LLMConnection::AuthType::AuthBearer: {
            m_authTypeCombo->setCurrentIndex(1);
            break;
        }
    }
    m_apiUrlEdit->setText(connection.apiUrl());
    m_apiKeyEdit->setText(connection.apiKey());
    m_isEnabledCheck->setChecked(connection.isEnabled());
    m_isDefaultCheck->setChecked(connection.isDefault());
}

inline LLMConnection LLMConnectionsDialog::getFormData() const
{
    LLMConnection connection;
    connection.setName(m_nameEdit->text().trimmed());
    connection.setProvider(m_providerCombo->currentText());
    connection.setApiUrl(m_apiUrlEdit->text().trimmed());
    connection.setApiKey(m_apiKeyEdit->text());
    if (m_authTypeCombo->currentIndex() >= LLMConnection::AuthType::AuthToken //
        && m_authTypeCombo->currentIndex() <= LLMConnection::AuthType::AuthBearer) {
        connection.setAuthType((LLMConnection::AuthType) m_authTypeCombo->currentIndex());
    }
    connection.setEnabled(m_isEnabledCheck->isChecked());
    connection.setDefault(m_isDefaultCheck->isChecked());
    return connection;
}

inline bool LLMConnectionsDialog::validateEditing() const
{
    bool ok = true;
    ok &= !m_nameEdit->text().trimmed().isEmpty();
    ok &= !m_apiUrlEdit->text().trimmed().isEmpty();
    return ok;
}

inline bool LLMConnectionsDialog::validateForm() const
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::critical((QWidget *) this, tr("Validation Error"), tr("Connection name is required."));
        m_nameEdit->setFocus();
        return false;
    }

    if (m_apiUrlEdit->text().trimmed().isEmpty()) {
        QMessageBox::critical((QWidget *) this, tr("Validation Error"), tr("API URL is required."));
        m_apiUrlEdit->setFocus();
        return false;
    }

    return true;
}

void LLMConnectionsDialog::showTestResult(const QString &result)
{
    m_statusLabel->setText(result);
    // Clear status after 5 seconds
    QTimer::singleShot(5000, this, [this]() { m_statusLabel->setText(QString()); });
}

void LLMConnectionsDialog::onFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QString errStr = reply->errorString();
        QTimer::singleShot(10, this, [this, errStr]() { //
            QMessageBox::critical((QWidget *) this, tr("Connection Error"), errStr);
        });
    } else {
        QTimer::singleShot(10, this, [this]() { //
            showTestResult(tr("Connection test successful. URL is valid."));
        });
    }

    reply->deleteLater();
}

void LLMConnectionsDialog::onError(QNetworkReply::NetworkError error)
{
    if (error != QNetworkReply::NoError) {
        QTimer::singleShot(10, this, [this]() {     //
            QMessageBox::critical((QWidget *) this, //
                                  tr("Connection Error"),
                                  tr("Unable to connect service."));
        });
    }
}

void LLMConnectionsDialog::onSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    if (reply->error() != QNetworkReply::NoError) {
        QStringList sl;
        foreach (auto e, errors) {
            sl.append(e.errorString());
        }
        QTimer::singleShot(10, this, [this, sl]() { //
            QMessageBox::critical((QWidget *) this, //
                                  tr("Connection Error"),
                                  sl.join("\n"));
        });
    }
}

void LLMConnectionsDialog::onAddClicked()
{
    m_tableView->setCurrentIndex(QModelIndex());
    m_isAdding = true;
    clearForm();
    updateButtons();
    m_nameEdit->setFocus();
}

void LLMConnectionsDialog::onEditClicked()
{
    if (!validateForm()) {
        return;
    }

    if (m_isAdding) {
        LLMConnection connection = getFormData();
        m_model->addConnection(connection);
        showTestResult(tr("Connection added successfully."));
    }

    if (!m_tableView->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndex currentIndex = m_tableView->selectionModel()->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    // Get the connection name from the selected row
    QModelIndex index = m_model->index(currentIndex.row(), 0);
    QString currentName = m_model->data(index, Qt::DisplayRole).toString();

    LLMConnection connection = getFormData();
    m_model->updateConnection(currentName, connection);

    showTestResult(tr("Connection updated successfully."));
}

void LLMConnectionsDialog::onRemoveClicked()
{
    if (!m_tableView->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndex currentIndex = m_tableView->selectionModel()->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    QString connectionName = m_model
                                 ->data( //
                                     m_model->index(currentIndex.row(), 0),
                                     Qt::DisplayRole)
                                 .toString();

    int reply = QMessageBox::question( //
        this,
        tr("Confirm Delete"),
        tr("Are you sure you want to delete connection '%1'?").arg(connectionName),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_model->removeConnection(connectionName);
        clearForm();
        loadConnections();
        showTestResult(tr("Connection removed successfully."));
    }
}

void LLMConnectionsDialog::onTestClicked()
{
    if (!m_tableView->selectionModel()->hasSelection()) {
        return;
    }

    QModelIndex currentIndex = m_tableView->selectionModel()->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    QString connectionName = m_model->data(m_model->index(currentIndex.row(), 0), Qt::DisplayRole).toString();
    LLMConnection connection = m_model->getConnection(connectionName);

    if (!connection.isEnabled()) {
        showTestResult(tr("Connection is disabled. Cannot test."));
        return;
    }

    // Show testing status
    showTestResult(tr("Testing connection..."));

    // For now, we'll simulate a test by checking if the URL is valid
    QString endpoint = connection.endpointUri(LLMConnection::EndpointModels);
    QString apiUrl = connection.apiUrl();
    if (!apiUrl.endsWith('/') && !endpoint.startsWith("/")) {
        apiUrl + "/" + endpoint;
    } else if (apiUrl.endsWith("/") && endpoint.startsWith("/")) {
        apiUrl = apiUrl + endpoint.mid(1);
    } else {
        apiUrl = apiUrl + endpoint;
    }

    QUrl url(apiUrl);
    if (!url.isValid()) {
        showTestResult(tr("Error: Invalid URL format."));
        return;
    }

    QNetworkRequest request(url);
    request.setTransferTimeout(2000);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!connection.apiKey().isEmpty()) {
        switch (connection.authType()) {
            case LLMConnection::AuthType::AuthToken: {
                request.setRawHeader("Authorization", "Token " + connection.apiKey().toUtf8());
                break;
            }
            case LLMConnection::AuthType::AuthBearer: {
                request.setRawHeader("Authorization", "Bearer " + connection.apiKey().toUtf8());
                break;
            }
        }
    }

    m_networkManager->get(request);
}

void LLMConnectionsDialog::onSelectionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    if (current.isValid()) {
        // Populate form with selected connection data
        QString connectionName = m_model->data(m_model->index(current.row(), 0), Qt::DisplayRole).toString();
        LLMConnection connection = m_model->getConnection(connectionName);
        populateForm(connection);
        m_isAdding = false;
    } else {
        clearForm();
    }
    updateButtons();
}

void LLMConnectionsDialog::onModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(topLeft);
    Q_UNUSED(bottomRight);
    Q_UNUSED(roles);
    // Refresh the view when data changes
    loadConnections();
    m_isAdding = false;
}

void LLMConnectionsDialog::onConnectionAdded(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(first);
    Q_UNUSED(last);
    // Refresh the view when data changes
    loadConnections();
    m_isAdding = false;
}

void LLMConnectionsDialog::onDialogRejected()
{
    reject();
}
