#pragma once
#include <attachbutton.h>
#include <chattextwidget.h>
#include <filelistwidget.h>
#include <llmchatclient.h>
#include <llmconnectionmodel.h>
#include <progresspopup.h>
#include <syntaxcolormodel.h>
#include <toolmodel.h>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

class ChatPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPanelWidget(LLMConnection *connection, SyntaxColorModel *scModel, ToolModel *tModel, QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

public slots:
    void onConnectionChanged(LLMConnection *connection);

private slots:
    void onUpdateChatText(int index, ChatMessage *message);
    void onToolRequest(ChatMessage *message, const ChatMessage::ToolEntry &tool);
    void onHideProgressPopup();
    void onShowProgressPopup();

private:
    // UI
    ChatTextWidget *m_chatView;
    QTextEdit *m_messageInput;
    QPushButton *m_sendButton;
    AttachButton *m_attachButton;
    // LLM connection data
    LLMConnection *m_llmConnection;
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

private:
    inline QWidget *createChatArea(QWidget *);
    inline QWidget *createInputArea(QWidget *);
    inline QWidget *createFileListWidget(QWidget *);
    inline QWidget *createInputWidget(QWidget *);
    inline QWidget *createLLMSelector(QWidget *);
    inline QWidget *createButtonBox(QWidget *);
    inline AttachButton *createAttachButton(QWidget *);
    inline QPushButton *createToolsButton(QWidget *);
    inline QPushButton *createSendButton(QWidget *);
    inline void reportLLMError(QNetworkReply::NetworkError error, const QString &message);
    inline void connectLLMClient();
    inline void connectChatModel();
};
