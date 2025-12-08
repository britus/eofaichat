#pragma once
#include <chatmodel.h>
#include <chattextwidget.h>
#include <llmchatclient.h>
#include <modellistmodel.h>
#include <progresspopup.h>
#include <syntaxcolormodel.h>
#include <QComboBox>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
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

    // Receive a JSON message as string and add to receiver bubble
    void receiveMessageJson(const QString &json);

private slots:
    void onSendClicked();
    void onAttachClicked();
    void onToolsClicked();
    void onResourcesClicked();
    void onModelSelected(int index);

private:
    QVBoxLayout *messagesLayout;
    QWidget *messagesContainer;
    QScrollArea *scrollArea;

    QTextEdit *messageInput;
    QPushButton *sendButton;
    QPushButton *attachButton;
    QPushButton *toolsButton;

    // Language model selection widget
    QWidget *modelSelectionWidget;
    QLabel *modelLabel;
    QComboBox *modelComboBox;

    LLMChatClient *llmclient;
    ModelListModel *llmModels;
    ChatModel *chatModel;
    ChatTextWidget *chatWidget;

    // Syntax color model used by all ChatTextWidget instances
    SyntaxColorModel *syntaxModel;

    // Selected model entry
    ModelEntry selectedModelEntry;

    QString lastDirectory;

    // Progress popup widget
    ProgressPopup *progressPopup;

    // Show/hide progress popup methods
    void showProgressPopup();
    void hideProgressPopup();
};