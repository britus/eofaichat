#pragma once
#include <chattexttokenizer.h>
#include <syntaxcolormodel.h>
#include <QObject>
#include <QTextEdit>

class ChatTextWidget : public QTextEdit
{
    Q_OBJECT
public:
    explicit ChatTextWidget(QWidget *parent = nullptr, SyntaxColorModel *model = nullptr);
    // Set/append a Markdown message. If isSender true -> sender layout will be applied.
    void setMessage(const QString &markdown, bool isSender);
    // Setter/getter SyntaxColorModel if needed
    void setSyntaxColorModel(SyntaxColorModel *model);

signals:
    // Signal emitted when the text document has been updated
    void documentUpdated();

private:
    // append given rendered document fragment or text, and attach highlighter for any code blocks
    void appendMarkdown(const QString &markdown, bool isSender);
    // Tokenize code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code, const QString &language);
    // Convert tokens to HTML with syntax highlighting
    QString tokensToHtml(const QVector<Token> &tokens, const QString &language, SyntaxColorModel *model);

private:
    SyntaxColorModel *m_colorModel;
};
