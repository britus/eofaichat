#pragma once
#include <QHBoxLayout>
#include <QMainWindow>
#include <QSettings>
#include <QWidget>

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

private:
    QHBoxLayout *mainLayout;
    QWidget *chatContainer;
    QWidget *centralWidget;
    QSettings *settings;
};
