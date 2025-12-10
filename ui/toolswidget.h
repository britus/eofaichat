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
    explicit ToolsWidget(QWidget *parent = nullptr);
    void setToolModel(ToolModel *model);

private slots:
    void refreshTool(int index);
    void showToolOptions(int index);
    void updateToolOption(int index, ToolOption option);

private:
    void setupUI();
    void populateToolBox();
    QWidget *createToolPage(ToolType type);
    QWidget *createToolBar(int index);
    QWidget *createToolItem(int index);
    void centerWindow();

    QToolBox *m_toolBox;
    ToolModel *m_model;
};

#endif // TOOLSWIDGET_H