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
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGridLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
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

private slots:
    void onSendClicked();
    void onAttachClicked();
    void onToolsClicked();
    void onResourcesClicked();
    void onModelSelected(int index);
    void onFileAdded(const QString &filePath);
    void onFileRemoved(int index);
    void onFileNameLabelRemoveClicked(int index);
    void onAttachButtonFileDropped(const QList<QUrl> &urls);

private:
    QVBoxLayout *messagesLayout;
    QWidget *messagesContainer;
    QWidget *chatWidgetContainer;
    QScrollArea *scrollArea;

    QTextEdit *messageInput;
    QPushButton *sendButton;
    AttachButton *attachButton;
    QPushButton *toolsButton;

    // Language model selection widget
    QWidget *modelSelectionWidget;
    QLabel *modelLabel;
    QComboBox *modelComboBox;

    LLMChatClient *llmclient;
    ModelListModel *llmModels;
    ChatModel *chatModel;
    //ChatTextWidget *chatWidget;
    // list messages with their own bubble widget
    ChatTextWidget *chatWidget;
#if 0
    QMap<ChatMessage *, ChatTextWidget *> m_messages;
#endif
    // Syntax color model used by all ChatTextWidget instances
    SyntaxColorModel *syntaxModel;

    // Selected model entry
    ModelEntry selectedModelEntry;

    QString lastDirectory;

    // Progress popup widget
    ProgressPopup *progressPopup;

    // File list model and widget
    FileListModel *fileListModel;
    FileListWidget *fileListWidget;

    // Show/hide progress popup methods
    void showProgressPopup();
    void hideProgressPopup();

    // Helper method to update file list widget visibility
    void updateFileListWidgetVisibility();
};
