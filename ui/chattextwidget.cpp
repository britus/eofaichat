#include <chattexttokenizer.h>
#include <chattextwidget.h>
#include <QBrush>
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

static inline void saveDocument(QTextDocument *document)
{
#if defined(QT_DEBUG)
    QDir home(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    QFile file(home.absoluteFilePath("_html_output.html"));
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        file.write(document->toHtml().toUtf8());
        file.close();
    }
#endif
}

ChatTextWidget::ChatTextWidget(QWidget *parent, SyntaxColorModel *model)
    : QTextEdit(parent)
    , m_colorModel(model)
{
    setReadOnly(true);
    setAcceptRichText(false); // Changed to false since we'll use QTextDocument formatting
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setMouseTracking(true);

    connect(this, &ChatTextWidget::linkActivated, this, [](const QUrl &url) {
        if (url.scheme() == "chat") {
            if (url.host() == "copy") { /* ... */
                qDebug("[ChatTextWidget] copy-url: %s", qPrintable(url.toDisplayString()));
            }
            if (url.host() == "remove") { /* ... */
                qDebug("[ChatTextWidget] remove-url: %s", qPrintable(url.toDisplayString()));
            }
        }
    });
}

void ChatTextWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QTextEdit::mouseReleaseEvent(event);
        return;
    }

    QTextCursor cursor = cursorForPosition(event->pos());
    QTextCharFormat fmt = cursor.charFormat();

    if (!fmt.isAnchor()) {
        QTextEdit::mouseReleaseEvent(event);
        return;
    }

    const QUrl url(fmt.anchorHref());
    if (!url.isValid()) {
        QTextEdit::mouseReleaseEvent(event);
        return;
    }

    QTextBlock block = cursor.block();
    auto *data = static_cast<BlockData *>(block.userData());

    emit linkActivated(url, data ? data->message() : nullptr);

    event->accept();
}

void ChatTextWidget::setSyntaxColorModel(SyntaxColorModel *model)
{
    m_colorModel = model;
}

void ChatTextWidget::removeMessage(ChatMessage *message)
{
    Q_UNUSED(message)
}

#if 0
void ChatTextWidget::updateMessage(ChatMessage *message)
{
    if (!message || message->content().isEmpty()) {
        return;
    }

    QTextDocument *doc = document();
    QTextBlock block = doc->begin();

    while (block.isValid()) {
        BlockData *data = static_cast<BlockData *>(block.userData());
        if (data && data->message() == message) {
            QTextCursor cursor(block);

            // Select and replace the entire block content with message content
            cursor.select(QTextCursor::BlockUnderCursor);
            cursor.beginEditBlock();
            cursor.removeSelectedText();
            cursor.insertText(message->content());
            cursor.endEditBlock();

            // Re-attach the BlockData to preserve the message reference
            block.setUserData(new BlockData(message));

            emit documentUpdated();
            return;
        }
        block = block.next();
    }

    // Emit signal when document is complete
    if (message->finishReason() == "stop") {
        emit documentUpdated();
    }
}
#endif

void ChatTextWidget::appendMessage(ChatMessage *message)
{
    if (!message || message->content().isEmpty()) {
        return;
    }

    qDebug().noquote() << "[ChatTextWidget] adding:" //
                       << message->id() << "msg:" << message->content();

    // user question and tool information
    if (message->role() == ChatMessage::ChatRole    //
        || message->role() == ChatMessage::UserRole //
        || message->role() == ChatMessage::ToolingRole) {
        appendMarkdown(message);
        saveDocument(document());
        emit documentUpdated();
    }
    // Add message to document if message stream has completed
    else if (message->finishReason() == "stop") {
        appendMarkdown(message);
        saveDocument(document());
        emit documentUpdated();
    }
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

static void attachBlockData(QTextCursor *cursor, ChatMessage *message)
{
    QTextBlock block = cursor->block();
    if (block.isValid()) {
        block.setUserData(new BlockData(message));
    }
}

// TODO: Insert 'menu' as html content???
inline void ChatTextWidget::insertActionMenu(QTextCursor *cursor, ChatMessage *message, Qt::Alignment alignment)
{
#if 0
    // Ensure text block appended at end of document
    cursor->movePosition(QTextCursor::End);

    QTextBlockFormat blockFmt;
    blockFmt.setAlignment(alignment);
    blockFmt.setTopMargin(2);
    blockFmt.setBottomMargin(2);
    blockFmt.setLeftMargin(0);
    blockFmt.setRightMargin(0);

    QTextCharFormat linkFmt;
    linkFmt.setFontPointSize(12);
    linkFmt.setForeground(QColor("#9aa0a6")); // subtle gray
    linkFmt.setAnchor(true);
    linkFmt.setUnderlineStyle(QTextCharFormat::NoUnderline);

    cursor->insertBlock(blockFmt);
    attachBlockData(cursor, message);

    // Copy
    QTextCharFormat copyFmt = linkFmt;
    copyFmt.setAnchorHref("chat://copy");
    cursor->insertText(QStringLiteral("📋 Copy"), copyFmt);
    cursor->insertText(QStringLiteral("  "), copyFmt);

    // Remove
    QTextCharFormat removeFmt = linkFmt;
    removeFmt.setAnchorHref("chat://remove");
    cursor->insertText(QStringLiteral("🗑 Remove"), removeFmt);
    cursor->insertText(QStringLiteral("  "), removeFmt);

    cursor->insertBlock(); // hard separation
#else
    Q_UNUSED(cursor)
    Q_UNUSED(message)
    Q_UNUSED(alignment)
#endif
}

static QStringList fontFamilies = QStringList() << "Menlo" << "monospace";

inline void ChatTextWidget::appendSeparator(QTextCursor *cursor)
{
    // Ensure text block appended at end of document
    cursor->movePosition(QTextCursor::End);

    QTextBlockFormat blockFmt;
    blockFmt.setTopMargin(0);
    blockFmt.setBottomMargin(0);
    blockFmt.setAlignment(Qt::AlignCenter);
    cursor->insertBlock(blockFmt);

    cursor->beginEditBlock();
    cursor->insertHtml(R"(<hr style=\"border: 0; border-top: 1px solid blue; height: 0; margin: 0; padding: 0; width: 100%;\">)");
    cursor->endEditBlock();
}

inline void ChatTextWidget::appendNormalText(QTextCursor *cursor, ChatMessage *message, const QString &normalBuffer)
{
    if (normalBuffer.isEmpty())
        return;

    // create temp markdown text document
    QTextDocument tmpDoc;
    // reset markdown character format from LLM
    tmpDoc.setMarkdown(normalBuffer, QTextDocument::MarkdownDialectGitHub);
    // extract fragment WITHOUT regenerating HTML
    QTextDocumentFragment fragment(&tmpDoc);

    // setup block and character format
    QTextBlockFormat blockFmt;
    blockFmt.setAlignment(message->role() == ChatMessage::ChatRole ? Qt::AlignRight : Qt::AlignLeft);
    blockFmt.setLeftMargin(0);
    blockFmt.setRightMargin(0);
    blockFmt.setTopMargin(12);
    blockFmt.setBottomMargin(12);

    QTextCharFormat charFmt;
    charFmt.setFontFamilies(fontFamilies);
    charFmt.setFontPointSize(16);
    charFmt.setForeground(Qt::white);

    // Ensure new text block appended at document end
    cursor->movePosition(QTextCursor::End);
    cursor->insertBlock(blockFmt, charFmt);

    // Assign LLM message to text block
    attachBlockData(cursor, message);

    // enforce char format BEFORE inserting fragment
    cursor->setCharFormat(charFmt);

    // Insert markdown content
    cursor->beginEditBlock();
    cursor->insertFragment(fragment);
    cursor->endEditBlock();

    // Insert 'menu' as html content
    insertActionMenu(cursor,
                     message,
                     message->role() == ChatMessage::ChatRole //
                         ? Qt::AlignRight
                         : Qt::AlignLeft);
}

inline void ChatTextWidget::appendCodeBlock(QTextCursor *cursor, ChatMessage *message, const QString &codeLang, const QString &codeBuffer)
{
    // Process code with proper newlines
    QVector<Token> tokens = tokenizeCode(codeBuffer, codeLang);
    QString htmlColored = tokensToHtml(tokens, codeLang, m_colorModel);

    // Create a block format for the code block
    QTextBlockFormat codeBlockFmt;
    codeBlockFmt.setAlignment(message->role() == ChatMessage::ChatRole ? Qt::AlignRight : Qt::AlignLeft);
    codeBlockFmt.setLeftMargin(0);
    codeBlockFmt.setRightMargin(0);
    codeBlockFmt.setTopMargin(12);
    codeBlockFmt.setBottomMargin(12);

    // Create character format for the code text
    QTextCharFormat codeCharFmt;
    codeCharFmt.setFontFamilies(fontFamilies);
    codeCharFmt.setFontPointSize(16);
    codeCharFmt.setForeground(Qt::white);

    // Ensure text block appended at end of document
    cursor->movePosition(QTextCursor::End);
    cursor->insertBlock(codeBlockFmt, codeCharFmt);

    // Assign LLM message to text block
    attachBlockData(cursor, message);

    // Insert HTML content
    cursor->beginEditBlock();
    cursor->insertHtml(htmlColored);
    cursor->endEditBlock();

    // Insert 'menu' as html content
    insertActionMenu(cursor,
                     message,
                     message->role() == ChatMessage::ChatRole //
                         ? Qt::AlignRight
                         : Qt::AlignLeft);
}

void ChatTextWidget::appendMarkdown(ChatMessage *message)
{
    // Use raw regex string for better readability
    const QRegularExpression startRe(R"(^(```(\s*([A-Za-z0-9_+\-=#]*))\s*$))");
    QString normalBuffer;

    QTextCursor cursor = textCursor();
    cursor.setVisualNavigation(true);

    if (!document()->isEmpty() && message->role() == ChatMessage::ChatRole) {
        appendSeparator(&cursor);
    }

    QString markdown = message->content();
    if (markdown.contains("\n\n")) {
        markdown = markdown.replace("\n\n", "\n");
    }
    if (message->role() == ChatMessage::SystemRole) {
        markdown = "```system\n" + markdown + "\n```";
    } else if (message->role() == ChatMessage::ToolingRole) {
        markdown = "```system\n" + markdown + "\n```";
    } else if (markdown.startsWith("{") && markdown.endsWith("}")) {
        markdown = "```json\n" + markdown + "\n```";
    } else if (markdown.startsWith("[") && markdown.endsWith("]")) {
        markdown = "```json\n" + markdown + "\n```";
    }

    // Ensure text block appended at the end of document
    cursor.movePosition(QTextCursor::End);

    QString codeLang;
    QString codeBuffer;
    QStringList lines = markdown.split('\n');
    bool inCode = false;

    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines[i];

        if (!inCode) {
            QRegularExpressionMatch match = startRe.match(line.trimmed());
            if (match.hasMatch()) {
                appendNormalText(&cursor, message, normalBuffer);
                normalBuffer.clear();
                // --
                inCode = true;
                codeLang = match.captured(2).toLower().trimmed(); // language part
                codeBuffer.clear();
                // for better separation
                //codeBuffer.append("\n\n");
                continue;
            } else {
                normalBuffer += line;
                if (i != lines.size() - 1) {
                    normalBuffer += '\n';
                }
            }
        } else {
            // Check code block end tag
            if (!line.trimmed().endsWith("```")) {
                codeBuffer += line;
                if (i != lines.size() - 1) {
                    codeBuffer += '\n';
                }
                continue;
            }

            // code block finished
            inCode = false;

            // for better separation
            //codeBuffer += '\n';
            appendCodeBlock(&cursor, message, codeLang, codeBuffer);

            codeBuffer.clear();
            codeLang.clear();
        }
    }

    if (!normalBuffer.isEmpty()) {
        appendNormalText(&cursor, message, normalBuffer);
        normalBuffer.clear();
    }

    // move to end
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}
