#ifndef TOOLSWIDGET_H
#define TOOLSWIDGET_H

#include "models/toolmodel.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QModelIndex>
#include <QToolBox>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

class ToolsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ToolsWidget(ToolModel *model, QWidget *parent = nullptr);

private slots:
    void refreshTool(int index);
    void showToolOptions(int index);
    void updateToolOption(int index, ToolModel::ToolOption option);
    void selectAllTools(int pageIndex);
    void deselectAllTools(int pageIndex);

private:
    void setupUI();
    void populateToolBox();
    QWidget *createToolPage(ToolModel::ToolType type);
    QWidget *createToolbar(int index);
    QWidget *createToolItem(int index);
    void centerWindow();
    void selectAllCheckboxes(QWidget *pageWidget, bool checked);

    QToolBox *m_toolBox;
    ToolModel *m_model;
};

#endif // TOOLSWIDGET_H
