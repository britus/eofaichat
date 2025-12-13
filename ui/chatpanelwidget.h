#pragma once
#include <attachbutton.h>
#include <chatmodel.h>
#include <chattextwidget.h>
#include <filelistmodel.h>
#include <filelistwidget.h>
#include <filenamelabel.h>
#include <llmchatclient.h>
#include <modellistmodel.h>
#include <progresspopup.h>
#include <syntaxcolormodel.h>
#include <toolmodel.h>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGridLayout>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMimeData>
#include <QPushButton>
#include <QScrollArea>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class ChatPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPanelWidget(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onUpdateChatText(int index, ChatMessage *message, bool removed = false);
    void onToolRequest(ChatMessage *message, const ChatMessage::ToolEntry &tool);
    void onHideProgressPopup();
    void onShowProgressPopup();

private:
    QVBoxLayout *messagesLayout;
    QWidget *messagesContainer;
    QWidget *chatWidgetContainer;
    QScrollArea *scrollArea;
    QTextEdit *messageInput;
    QPushButton *sendButton;

    LLMChatClient *llmclient;
    ChatModel *chatModel;

    // list messages with their own bubble widget
    ChatTextWidget *chatWidget;
#if 0
    QMap<ChatMessage *, ChatTextWidget *> m_messages;
#endif
    // Syntax color model used by all ChatTextWidget instances
    SyntaxColorModel *syntaxModel;

    QString lastDirectory;
    QMainWindow *toolsWindow;

    // Progress popup widget
    ProgressPopup *progressPopup;

    // File list model and widget
    FileListModel *fileListModel;
    FileListWidget *fileListWidget;

    // LLM Tooling
    ToolModel *toolModel;

private:
    inline void createChatArea(QVBoxLayout *);
    inline void createFileListWidget(QVBoxLayout *);
    inline void createInputWidget(QVBoxLayout *);
    inline void createLLMSelector(QVBoxLayout *);
    inline void createAttachButton(QHBoxLayout *);
    inline void createToolsButton(QHBoxLayout *);
    inline void createSendButton(QHBoxLayout *);
    inline void connectLLMClient();
    inline void connectChatModel();
};
