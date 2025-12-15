#pragma once
#include <QAction>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QWidget>

class SettingsManager;
class LLMConnectionModel;
class LLMConnectionsDialog;
class SyntaxColorModel;
class ToolModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    static MainWindow *window();

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Expose central widget for blur effect
    inline QWidget *centralWidget() const { return m_centralWidget; }
    inline QWidget *contentWidget() const { return m_contentWidget; }
    inline SyntaxColorModel *syntaxModel() const { return m_syntaxModel; }
    inline ToolModel *toolModel() const { return m_toolModel; }
    inline SettingsManager *settings() const { return m_settingsManager; }

private slots:
    void onChatSelected(QWidget *chatWidget);
    void onChatRemoved(QWidget *chatWidget);
    void onManageConnections();

private:
    void setupMenuBar();

private:
    QMenu *m_toolsMenu;
    QHBoxLayout *m_mainLayout;
    QWidget *m_contentWidget;
    QWidget *m_centralWidget;
    SettingsManager *m_settingsManager;
    LLMConnectionModel *m_connectionModel;
    SyntaxColorModel *m_syntaxModel;
    ToolModel *m_toolModel;
};
