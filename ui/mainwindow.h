#pragma once
#include <QHBoxLayout>
#include <QMainWindow>
#include <QSettings>
#include <QWidget>
#include <QMenu>
#include <QMenuBar>
#include <QAction>

class SettingsManager;
class LLMConnectionModel;
class LLMConnectionsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Expose central widget for blur effect
    QWidget *getCentralWidget() const;

private slots:
    void onChatSelected(QWidget *chatWidget);
    void onChatRemoved(QWidget *chatWidget);
    void onManageConnections();

private:
    void setupMenuBar();
    
private:
    QHBoxLayout *mainLayout;
    QWidget *chatContainer;
    QWidget *centralWidget;
    SettingsManager *settingsManager;
    LLMConnectionModel *m_connectionModel;
    QMenu *m_toolsMenu;
};