#include "toolswidget.h"
#include <QCheckBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QScrollArea>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

ToolsWidget::ToolsWidget(QWidget *parent)
    : QWidget(parent)
    , m_toolBox(new QToolBox(this))
    , m_model(nullptr)
{
    setupUI();
    populateToolBox();
}

void ToolsWidget::setToolModel(ToolModel *model)
{
    m_model = model;
    populateToolBox();
}

void ToolsWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_toolBox);
    setLayout(mainLayout);
}

void ToolsWidget::populateToolBox()
{
    if (!m_model)
        return;

    // Clear existing pages
    while (m_toolBox->count() > 0)
        m_toolBox->removeItem(0);

    // Create pages for each tool type
    m_toolBox->addItem(createToolPage(Tool), tr("Tools"));
    m_toolBox->addItem(createToolPage(Resource), tr("Resources"));
    m_toolBox->addItem(createToolPage(Prompt), tr("Prompts"));
}

QWidget *ToolsWidget::createToolPage(ToolType type)
{
    // Create a scroll area to hold all the tool items
    QScrollArea *scrollArea = new QScrollArea();
    QWidget *pageWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(pageWidget);

    // Create container widget for the toolbar and tool items
    QWidget *containerWidget = new QWidget();
    QVBoxLayout *containerLayout = new QVBoxLayout(containerWidget);

    // Create toolbar for this page
    QWidget *toolbar = createToolBar(-1); // -1 indicates page-level toolbar
    containerLayout->addWidget(toolbar);

    // Add tool items based on model data for this specific type
    if (m_model) {
        for (int i = 0; i < m_model->rowCount(); ++i) {
            QModelIndex index = m_model->index(i);
            ToolType entryType = static_cast<ToolType>(m_model->data(index, ToolModel::TypeRole).toInt());
            if (entryType == type) {
                QWidget *toolItem = createToolItem(i);
                containerLayout->addWidget(toolItem);
            }
        }
    }

    // Add stretch to push items up
    containerLayout->addStretch();

    layout->addWidget(containerWidget);
    scrollArea->setWidget(pageWidget);
    scrollArea->setWidgetResizable(true);

    return scrollArea;
}

QWidget *ToolsWidget::createToolBar(int index)
{
    QWidget *toolbar = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(0, 0, 0, 0);

    // Refresh button
    QToolButton *refreshButton = new QToolButton(toolbar);
    refreshButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    refreshButton->setToolTip(tr("Refresh"));
    connect(refreshButton, &QToolButton::clicked, [this, index]() { refreshTool(index); });

    // Options button
    QToolButton *optionsButton = new QToolButton(toolbar);
    optionsButton->setText("...");
    optionsButton->setToolTip(tr("Options"));
    connect(optionsButton, &QToolButton::clicked, [this, index]() { showToolOptions(index); });

    // Add buttons to the right side
    layout->addStretch();
    layout->addWidget(optionsButton);
    layout->addWidget(refreshButton);

    return toolbar;
}

QWidget *ToolsWidget::createToolItem(int index)
{
    QWidget *widget = new QWidget();
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    // Checkbox for enabling/disabling tool
    QCheckBox *checkBox = new QCheckBox(widget);
    ToolOption option = static_cast<ToolOption>(m_model->data(m_model->index(index), ToolModel::OptionRole).toInt());
    checkBox->setChecked(option != ToolDisabled);
    connect(checkBox, &QCheckBox::clicked, [this, index](bool checked) {
        ToolOption newOption = checked ? ToolEnabled : ToolDisabled;
        updateToolOption(index, newOption);
    });

    // Label for tool name
    QLabel *nameLabel = new QLabel(m_model->data(m_model->index(index), ToolModel::NameRole).toString(), widget);

    // Tool button with popup menu
    QToolButton *toolButton = new QToolButton(widget);
    toolButton->setText("...");
    toolButton->setPopupMode(QToolButton::InstantPopup);

    QMenu *menu = new QMenu(toolButton);
    QAction *allowAction = menu->addAction(tr("Allow"));
    QAction *askBeforeRunAction = menu->addAction(tr("Ask before run"));

    connect(allowAction, &QAction::triggered, [this, index]() { updateToolOption(index, ToolEnabled); });
    connect(askBeforeRunAction, &QAction::triggered, [this, index]() { updateToolOption(index, AskBeforeRun); });

    toolButton->setMenu(menu);

    layout->addWidget(checkBox);
    layout->addWidget(nameLabel);
    layout->addStretch();
    layout->addWidget(toolButton);

    return widget;
}

void ToolsWidget::refreshTool(int index)
{
    // Implement refresh functionality
    qDebug() << "Refreshing tool at index:" << index;
}

void ToolsWidget::showToolOptions(int index)
{
    // Implement options functionality
    qDebug() << "Showing options for tool at index:" << index;
}

void ToolsWidget::updateToolOption(int index, ToolOption option)
{
    // Update the tool option in the model
    if (m_model) {
        QModelIndex modelIndex = m_model->index(index);
        m_model->setData(modelIndex, option, ToolModel::OptionRole);
    }
}
