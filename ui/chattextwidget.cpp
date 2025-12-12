#include <chattexttokenizer.h>
#include <chattextwidget.h>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTimer>

class BlockData : public QTextBlockUserData
{
public:
    explicit BlockData(ChatMessage *message)
        : QTextBlockUserData()
        , m_message(message)
    {}
    inline ChatMessage *message() { return m_message; };
    inline const QString &messageId() const { return m_message->id(); };

private:
    ChatMessage *m_message;
};

#if defined(QT_DEBUG)
static inline void saveDocument(QTextDocument *document)
{
    QDir home(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    QFile file(home.absoluteFilePath("_html_output.html"));
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        file.write(document->toHtml().toUtf8());
        file.close();
    }
}
#endif

ChatTextWidget::ChatTextWidget(QWidget *parent, SyntaxColorModel *model)
    : QTextEdit(parent)
    , m_colorModel(model)
{
    setReadOnly(true);
    setAcceptRichText(false); // Changed to false since we'll use QTextDocument formatting
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
}

void ChatTextWidget::setSyntaxColorModel(SyntaxColorModel *model)
{
    m_colorModel = model;
}

void ChatTextWidget::removeMessage(ChatMessage *message)
{
    Q_UNUSED(message)
}

void ChatTextWidget::updateMessage(ChatMessage *message)
{
    if (!message || message->content().isEmpty()) {
        return;
    }
    if (m_messages.contains(message->id())) {
        return;
    }

    appendMarkdown(message);

    // Emit signal when document is complete
    if (message->finishReason() == "stop") {
        emit documentUpdated();
    }

#if defined(QT_DEBUG)
    saveDocument(document());
#endif
}

void ChatTextWidget::appendMessage(ChatMessage *message)
{
    if (!message || message->content().isEmpty()) {
        return;
    }
    if (m_messages.contains(message->id())) {
        return;
    }

    appendMarkdown(message);

    // Emit signal when document is complete
    if (message->finishReason() == "stop") {
        emit documentUpdated();
    }

#if defined(QT_DEBUG)
    saveDocument(document());
#endif
}

QVector<Token> ChatTextWidget::tokenizeCode(const QString &code, const QString &language)
{
    // Delegate to the new ChatTextTokenizer class
    return ChatTextTokenizer::tokenizeCode(code, language);
}

QString ChatTextWidget::tokensToHtml(const QVector<Token> &tokens, const QString &language, SyntaxColorModel *model)
{
    // Delegate to the new ChatTextTokenizer class
    return ChatTextTokenizer::tokensToHtml(tokens, language, model);
}

void ChatTextWidget::appendMarkdown(ChatMessage *message)
{
    bool inCode = false;
    ChatMessage::Role role = message->role();
    QStringList fontFamilies = QStringList() << "Menlo" << "Consolas" << "monospace";
    QString codeLang;
    QString codeBuffer;
    QString normalBuffer;

    QString markdown = message->content();
    if (message->role() == ChatMessage::SystemRole) {
        markdown = "```system\n" + markdown + "\n```";
    } else if (markdown.startsWith("[") && markdown.endsWith("]")) {
        markdown = "```json\n" + markdown + "\n```";
    }

    QStringList lines = markdown.split('\n');
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);

    auto flushNormal = [&]() {
        if (normalBuffer.isEmpty())
            return;

        QTextDocument markdownDoc;
        markdownDoc.setMarkdown(normalBuffer, QTextDocument::MarkdownDialectGitHub);

        QTextBlockFormat blockFmt;
        blockFmt.setAlignment(role == ChatMessage::ChatRole ? Qt::AlignRight : Qt::AlignLeft);
        blockFmt.setLeftMargin(12);
        blockFmt.setRightMargin(12);
        blockFmt.setTopMargin(8);
        blockFmt.setBottomMargin(8);

        QTextCharFormat charFmt;
        charFmt.setFontFamilies(fontFamilies);
        charFmt.setFontPointSize(18);
        charFmt.setForeground(Qt::white);

        cursor.movePosition(QTextCursor::End);
        cursor.insertBlock(blockFmt, charFmt);

        cursor.beginEditBlock();
        cursor.insertHtml(markdownDoc.toHtml());
        cursor.endEditBlock();

        QTextBlock block = document()->lastBlock();
        cursor.setPosition(block.position());
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.setCharFormat(charFmt);

        normalBuffer.clear();
    };

    // Use raw regex string for better readability
    const QRegularExpression startRe(R"(^(```(\s*([A-Za-z0-9_+\-=#]*))\s*$))");

    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines[i];

        if (!inCode) {
            QRegularExpressionMatch match = startRe.match(line.trimmed());
            if (match.hasMatch()) {
                flushNormal();
                inCode = true;
                codeLang = match.captured(2).toLower().trimmed(); // language part
                codeBuffer.clear();
                continue;
            } else {
                normalBuffer += line;
                if (i != lines.size() - 1) {
                    normalBuffer += '\n';
                }
            }
        } else {
            // Check if this line ends the code block
            if (line.trimmed() == "```") {
                inCode = false;

                // Process code with proper newlines
                QVector<Token> tokens = tokenizeCode(codeBuffer, codeLang);
                QString htmlColored = tokensToHtml(tokens, codeLang, m_colorModel);

                // Create a block format for the code block
                QTextBlockFormat codeBlockFmt;
                codeBlockFmt.setAlignment(role == ChatMessage::ChatRole ? Qt::AlignRight : Qt::AlignLeft);
                codeBlockFmt.setLeftMargin(12);
                codeBlockFmt.setRightMargin(12);
                codeBlockFmt.setTopMargin(8);
                codeBlockFmt.setBottomMargin(8);

                // Create character format for the code text
                QTextCharFormat codeCharFmt;
                codeCharFmt.setFontFamilies(fontFamilies);
                codeCharFmt.setFontPointSize(18);
                codeCharFmt.setForeground(Qt::white);

                // Insert HTML content
                cursor.movePosition(QTextCursor::End);
                cursor.insertBlock(codeBlockFmt, codeCharFmt);
                cursor.beginEditBlock();
                cursor.insertHtml(htmlColored);
                cursor.endEditBlock();

                codeBuffer.clear();
                codeLang.clear();
            } else {
                codeBuffer += line;
                if (i != lines.size() - 1) {
                    codeBuffer += '\n';
                }
            }
        }
    }

    if (!normalBuffer.isEmpty()) {
        flushNormal();
    }

    m_messages.append(message->id());
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}
