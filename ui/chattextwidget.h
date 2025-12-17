#pragma once
#include <chatmessage.h>
#include <chattexttokenizer.h>
#include <syntaxcolormodel.h>
#include <QObject>
#include <QTextEdit>

class ChatTextWidget : public QTextEdit
{
    Q_OBJECT

public:
    explicit ChatTextWidget(QWidget *parent = nullptr, SyntaxColorModel *model = nullptr);
    // Setter/getter SyntaxColorModel if needed
    void setSyntaxColorModel(SyntaxColorModel *model);
    // LLM messages
    void appendMessage(ChatMessage *message);
    void removeMessage(ChatMessage *message);

signals:
    // Signal emitted when the text document has been updated
    void documentUpdated();
    // Signal emmitted on text links
    void linkActivated(const QUrl &url, ChatMessage *message);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // append given rendered document fragment or text, and attach highlighter for any code blocks
    void appendMarkdown(ChatMessage *message);
    // Tokenize code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code, const QString &language);
    // Convert tokens to HTML with syntax highlighting
    QString tokensToHtml(const QVector<Token> &tokens, const QString &language, SyntaxColorModel *model);
    inline void appendSeparator(QTextCursor *cursor);
    inline void appendNormalText(QTextCursor *cursor, ChatMessage *message, const QString &normalBuffer);
    inline void appendCodeBlock(QTextCursor *cursor, ChatMessage *message, const QString &codeLang, const QString &codeBuffer);
    inline void insertActionMenu(QTextCursor *cursor, ChatMessage *message, Qt::Alignment alignment);

private:
    SyntaxColorModel *m_colorModel;
};
