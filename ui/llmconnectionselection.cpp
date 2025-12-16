#include "llmconnectionselection.h"
#include "llmconnectionmodel.h"
#include <QApplication>
#include <QDebug>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

LLMConnectionSelection::LLMConnectionSelection(LLMConnectionModel *model, QWidget *parent)
    : QDialog(parent)
    , m_model(model)
    , m_selectedConnectionName(QString())
{
    setupUI();
    loadConnections();

    // Set window title
    setWindowTitle(tr("%1 - Select LLM Connection").arg(qApp->applicationDisplayName()));
    resize(400, 300);
}

LLMConnectionSelection::~LLMConnectionSelection()
{
    //-
}

void LLMConnectionSelection::setupUI()
{
    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Status label
    m_statusLabel = new QLabel(tr("Select a connection from the list below:"), this);
    mainLayout->addWidget(m_statusLabel);

    // List view for connections
    m_listView = new QListView(this);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setEditTriggers(QListView::NoEditTriggers);
    m_listView->setModel(m_model);
    mainLayout->addWidget(m_listView);

    // Button box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_cancelButton = buttonBox->button(QDialogButtonBox::Cancel);

    connect(m_okButton, &QPushButton::clicked, this, &LLMConnectionSelection::onOkClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &LLMConnectionSelection::onCancelClicked);
    connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &LLMConnectionSelection::onSelectionChanged);
    connect(m_listView, &QListView::doubleClicked, this, &LLMConnectionSelection::onItemDoubleClicked);

    mainLayout->addWidget(buttonBox);

    // Initially disable OK button
    m_okButton->setEnabled(false);
}

void LLMConnectionSelection::loadConnections()
{
    // The model is already loaded, we just need to make sure the view is updated
    m_listView->setModel(m_model);
}

QString LLMConnectionSelection::selectedConnectionName() const
{
    return m_selectedConnectionName;
}

void LLMConnectionSelection::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)

    if (!selected.isEmpty()) {
        QModelIndex index = selected.indexes().first();
        if (index.isValid()) {
            // Get the connection name from the model
            QString connectionName = m_model->data(m_model->index(index.row(), 0), Qt::DisplayRole).toString();
            m_selectedConnectionName = connectionName;
            m_okButton->setEnabled(true);
        }
    } else {
        m_selectedConnectionName.clear();
        m_okButton->setEnabled(false);
    }
}

void LLMConnectionSelection::onOkClicked()
{
    if (!m_selectedConnectionName.isEmpty()) {
        accept();
    }
}

void LLMConnectionSelection::onCancelClicked()
{
    m_selectedConnectionName.clear();
    reject();
}

void LLMConnectionSelection::onItemDoubleClicked(const QModelIndex &index)
{
    if (index.isValid()) {
        // Get the connection name from the model
        QString connectionName = m_model->data(m_model->index(index.row(), 0), Qt::DisplayRole).toString();
        m_selectedConnectionName = connectionName;
        accept();
    }
}