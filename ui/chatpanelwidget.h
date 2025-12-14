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
    explicit ChatPanelWidget(SyntaxColorModel *scModel, ToolModel *tModel, QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onUpdateChatText(int index, ChatMessage *message);
    void onToolRequest(ChatMessage *message, const ChatMessage::ToolEntry &tool);
    void onHideProgressPopup();
    void onShowProgressPopup();

private:
    // UI
    QVBoxLayout *m_messagesLayout;
    QWidget *m_messagesContainer;
    QScrollArea *m_scrollArea;
    QTextEdit *m_messageInput;
    QPushButton *m_sendButton;
    ChatTextWidget *m_chatWidget;
    FileListWidget *m_fileListWidget;
    AttachButton *m_attachButton;
    QMainWindow *m_toolsWindow;
    // LLM connection client
    LLMChatClient *m_llmclient;
    // Syntax color model used by all ChatTextWidget instances
    SyntaxColorModel *m_syntaxModel;
    // LLM Tooling
    ToolModel *m_toolModel;
    // File list model and widget
    FileListModel *m_fileListModel;
    // Progress popup widget
    ProgressPopup *m_progressPopup;
    // Attach file dialog
    QString m_lastDirectory;
    // on action
    bool m_isConversating;

#if 0
    QMap<ChatMessage *, ChatTextWidget *> m_messages;
#endif

private:
    inline void createChatArea(QVBoxLayout *);
    inline void createFileListWidget(QVBoxLayout *);
    inline void createInputWidget(QVBoxLayout *);
    inline void createLLMSelector(QVBoxLayout *);
    inline void createAttachButton(QHBoxLayout *);
    inline void createToolsButton(QHBoxLayout *);
    inline void createSendButton(QHBoxLayout *);
    inline void reportLLMError(QNetworkReply::NetworkError error, const QString &message);
    inline void connectLLMClient();
    inline void connectChatModel();
};
