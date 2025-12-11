#include "toolswidget.h"
#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QScrollArea>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

ToolsWidget::ToolsWidget(ToolModel *model, QWidget *parent)
    : QWidget(parent)
    , m_toolBox(new QToolBox(this))
    , m_model(model)
{
    setupUI();
}

void ToolsWidget::setupUI()
{
    m_toolBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_toolBox);
    setLayout(mainLayout);
    populateToolBox();
}

void ToolsWidget::populateToolBox()
{
    if (!m_model)
        return;

    // Clear existing pages
    while (m_toolBox->count() > 0)
        m_toolBox->removeItem(0);

    // Create pages for each tool type
    m_toolBox->addItem(createToolPage(ToolModel::Tool), tr("☞ Tools"));
    m_toolBox->addItem(createToolPage(ToolModel::Resource), tr("☞ Resources"));
    m_toolBox->addItem(createToolPage(ToolModel::Prompt), tr("☞ Prompts"));
}

QWidget *ToolsWidget::createToolPage(ToolModel::ToolType type)
{
    // Create a scroll area to hold all the tool items
    QWidget *pageWidget = new QWidget();
    pageWidget->setObjectName(QStringLiteral("toolPage%1").arg(type));

    QScrollArea *scrollArea = new QScrollArea();
    QVBoxLayout *layout = new QVBoxLayout(pageWidget);

    // Create container widget for the toolbar and tool items
    QWidget *containerWidget = new QWidget();
    containerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *containerLayout = new QVBoxLayout(containerWidget);

    // Create toolbar for this page
    // We'll pass the index of the page in the tool box to identify it later
    int pageIndex = m_toolBox->count();
    QWidget *toolbar = createToolbar(pageIndex);
    containerLayout->addWidget(toolbar);

    // Add tool items based on model data for this specific type
    if (m_model) {
        for (int i = 0; i < m_model->rowCount(); ++i) {
            QModelIndex index = m_model->index(i);
            ToolModel::ToolType entryType = static_cast<ToolModel::ToolType>(m_model->data(index, ToolModel::TypeRole).toInt());
            if (entryType == type) {
                QWidget *toolItem = createToolItem(i);
                containerLayout->addWidget(toolItem);
            }
        }
    }

    // Add stretch to push items up
    containerLayout->addStretch();
    layout->addWidget(containerWidget);
    layout->addStretch();

    scrollArea->setWidget(pageWidget);
    scrollArea->setWidgetResizable(true);

    return scrollArea;
}

QWidget *ToolsWidget::createToolbar(int index)
{
    QWidget *toolbar = new QWidget();
    toolbar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    toolbar->setObjectName(QStringLiteral("toolPageToolbar%1").arg(index));

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
    // Create menu for options button
    QMenu *optionsMenu = new QMenu(optionsButton);
    QAction *selectAllAction = optionsMenu->addAction(tr("Select all"));
    QAction *deselectAllAction = optionsMenu->addAction(tr("Deselect all"));

    // Connect menu actions to slots with the page index
    connect(selectAllAction, &QAction::triggered, [this, index]() { selectAllTools(index); });
    connect(deselectAllAction, &QAction::triggered, [this, index]() { deselectAllTools(index); });

    optionsButton->setMenu(optionsMenu);
    optionsButton->setPopupMode(QToolButton::InstantPopup);

    // Add buttons to the right side
    layout->addStretch();
    layout->addWidget(optionsButton);
    layout->addWidget(refreshButton);

    return toolbar;
}

QWidget *ToolsWidget::createToolItem(int index)
{
    QWidget *widget = new QWidget();
    widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    widget->setObjectName(QStringLiteral("toolItem%1").arg(index));

    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    // Checkbox for enabling/disabling tool
    QCheckBox *checkBox = new QCheckBox(widget);
    ToolModel::ToolOption option = static_cast<ToolModel::ToolOption>(m_model->data(m_model->index(index), ToolModel::OptionRole).toInt());
    checkBox->setChecked(option != ToolModel::ToolDisabled);

    // Store the index with the checkbox for later use
    checkBox->setProperty("toolIndex", index);

    // Connect checkbox to update tool option
    connect(checkBox, &QCheckBox::clicked, [this, index](bool checked) {
        ToolModel::ToolOption newOption = checked ? ToolModel::ToolEnabled : ToolModel::ToolDisabled;
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

    // Connect menu actions to update tool option
    connect(allowAction, &QAction::triggered, [this, index]() { updateToolOption(index, ToolModel::ToolEnabled); });
    connect(askBeforeRunAction, &QAction::triggered, [this, index]() { updateToolOption(index, ToolModel::AskBeforeRun); });

    // Set the initial state of menu actions based on current tool option
    if (option == ToolModel::ToolEnabled) {
        allowAction->setChecked(true);
    } else if (option == ToolModel::AskBeforeRun) {
        askBeforeRunAction->setChecked(true);
    }

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

void ToolsWidget::updateToolOption(int index, ToolModel::ToolOption option)
{
    // Update the tool option in the model
    if (m_model) {
        QModelIndex modelIndex = m_model->index(index);
        m_model->setData(modelIndex, option, ToolModel::OptionRole);
    }
}

void ToolsWidget::selectAllTools(int pageIndex)
{
    // Get the page widget for the specified page index
    QWidget *pageWidget = m_toolBox->widget(pageIndex);
    if (pageWidget) {
        // Find all QCheckBoxes in the page widget and set them to checked
        QList<QCheckBox *> checkBoxes = pageWidget->findChildren<QCheckBox *>();
        foreach (QCheckBox *checkBox, checkBoxes) {
            if (checkBox->isEnabled() && !checkBox->isChecked()) {
                // Get the tool index from the property
                int toolIndex = checkBox->property("toolIndex").toInt();
                // Update the checkbox state
                checkBox->setChecked(true);
                // Update the model through our existing function
                updateToolOption(toolIndex, ToolModel::ToolEnabled);
            }
        }
    }
}

void ToolsWidget::deselectAllTools(int pageIndex)
{
    // Get the page widget for the specified page index
    QWidget *pageWidget = m_toolBox->widget(pageIndex);
    if (pageWidget) {
        // Find all QCheckBoxes in the page widget and set them to unchecked
        QList<QCheckBox *> checkBoxes = pageWidget->findChildren<QCheckBox *>();
        foreach (QCheckBox *checkBox, checkBoxes) {
            if (checkBox->isEnabled() && checkBox->isChecked()) {
                // Get the tool index from the property
                int toolIndex = checkBox->property("toolIndex").toInt();
                // Update the checkbox state
                checkBox->setChecked(false);
                // Update the model through our existing function
                updateToolOption(toolIndex, ToolModel::ToolDisabled);
            }
        }
    }
}
