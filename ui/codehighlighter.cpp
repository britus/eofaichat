#include <codehighlighter.h>
#include <QDebug>

CodeHighlighter::CodeHighlighter(QTextDocument *parent, const QString &language, SyntaxColorModel *model)
    : QSyntaxHighlighter(parent)
    , m_language(language.toLower())
    , m_model(model)
{
    defaultFormat.setFontFamily("monospace");
    defaultFormat.setFontPointSize(11);
    setupRulesForLanguage(language);
}

void CodeHighlighter::setupRulesForLanguage(const QString &language)
{
    rules.clear();

    // Helper to create a format from model token key
    auto fmt = [&](const QString &tokenKey) {
        QTextCharFormat f;
        QColor c = m_model ? m_model->colorFor(language, tokenKey, QColor("#cccccc")) : QColor("#cccccc");
        f.setForeground(c);
        f.setFontFamily("monospace");
        return f;
    };

    // Very simple patterns per language — extend as needed.
    if (language.startsWith("c") || language == "cpp" || language == "c++") {
        // keywords
        QTextCharFormat kw = fmt("keyword");
        QRegularExpression kwre("\\b(alignas|alignof|and|and_eq|asm|auto|bool|break|case|catch|char|class|const|constexpr|continue|decltype|default|delete|do|double|else|enum|explicit|export|extern|"
                                "false|final|float|for|friend|goto|if|inline|int|long|mutable|namespace|new|noexcept|nullptr|operator|private|protected|public|register|override|return|short|signed|"
                                "sizeof|static|struct|switch|template|this|throw|true|try|typedef|typeid|typename|union|unsigned|using|virtual|void|volatile|while)\\b");
        rules.append({kwre, kw});
        // single-line comment
        QTextCharFormat com = fmt("comment");
        rules.append({QRegularExpression("//[^\n]*"), com});
        // multi-line comment handled in highlightBlock via setCurrentBlockState
        // strings
        QTextCharFormat str = fmt("string");
        rules.append({QRegularExpression(R"("(?:\\.|[^"\\])*")"), str});
        // numbers
        QTextCharFormat numb = fmt("number");
        rules.append({QRegularExpression("\\b[0-9]+(\\.[0-9]+)?\\b"), numb});
    } else if (language == "java") {
        QTextCharFormat kw = fmt("keyword");
        rules.append(
            {QRegularExpression(
                 "\\b(abstract|assert|boolean|break|byte|case|catch|char|class|const|continue|default|do|double|else|enum|extends|final|finally|float|for|goto|if|implements|import|instanceof|int|"
                 "interface|long|native|new|null|package|private|protected|public|return|short|static|strictfp|super|switch|synchronized|this|throw|throws|transient|true|try|void|volatile|while)\\b"),
             kw});
        rules.append({QRegularExpression("//[^\n]*"), fmt("comment")});
        rules.append({QRegularExpression(R"("(?:\\.|[^"\\])*")"), fmt("string")});
    } else if (language == "javascript" || language == "typescript" || language == "ts") {
        QTextCharFormat kw = fmt("keyword");
        rules.append({QRegularExpression("\\b(break|case|catch|class|const|continue|debugger|default|delete|do|else|export|extends|finally|for|function|if|import|in|instanceof|let|new|null|return|"
                                         "super|switch|this|throw|try|typeof|var|void|while|with|yield|await)\\b"),
                      kw});
        rules.append({QRegularExpression("//[^\n]*"), fmt("comment")});
        rules.append({QRegularExpression(R"('(?:\\.|[^'\\])*')"), fmt("string")});
        rules.append({QRegularExpression(R"("(?:\\.|[^"\\])*")"), fmt("string")});
        rules.append({QRegularExpression("\\b[0-9]+(\\.[0-9]+)?\\b"), fmt("number")});
    } else if (language == "sql" || language == "ansi sql" || language == "sql") {
        rules.append({QRegularExpression("\\b(SELECT|FROM|WHERE|INSERT|INTO|VALUES|UPDATE|SET|DELETE|JOIN|LEFT|RIGHT|INNER|OUTER|ON|AS|GROUP BY|ORDER BY|HAVING|LIMIT|OFFSET)\\b",
                                         QRegularExpression::CaseInsensitiveOption),
                      fmt("keyword")});
        rules.append({QRegularExpression("--[^\n]*"), fmt("comment")});
        rules.append({QRegularExpression(R"('(?:''|[^'])*')"), fmt("string")});
    } else if (language == "bash" || language == "sh") {
        rules.append({QRegularExpression("#[^\\n]*"), fmt("comment")});
        rules.append({QRegularExpression("\\$[A-Za-z_][A-Za-z0-9_]*"), fmt("variable")});
        rules.append({QRegularExpression("\\b(if|then|else|fi|for|while|in|do|done|case|esac|function)\\b"), fmt("keyword")});
        rules.append({QRegularExpression(R"("(?:\\.|[^"\\])*")"), fmt("string")});
    } else {
        // fallback basic rules
        rules.append({QRegularExpression("//[^\n]*"), fmt("comment")});
        rules.append({QRegularExpression(R"("(?:\\.|[^"\\])*")"), fmt("string")});
    }
}

void CodeHighlighter::highlightBlock(const QString &text)
{
    // Apply default format (monospace) to whole block
    setCurrentBlockState(0);

    foreach (const Rule &r, qAsConst(rules)) {
        QRegularExpressionMatchIterator it = r.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            int start = match.capturedStart();
            int len = match.capturedLength();
            setFormat(start, len, r.format);
        }
    }

    // Multi-line comment support for C-like languages (/* ... */)
    if (m_language.startsWith("c") || m_language == "cpp" || m_language == "c++" || m_language == "java") {
        QRegularExpression startDelim("/\\*");
        QRegularExpression endDelim("\\*/");

        int startIndex = 0;
        if (previousBlockState() != 1)
            startIndex = text.indexOf(startDelim);

        while (startIndex >= 0) {
            QRegularExpressionMatch endMatch = endDelim.match(text, startIndex);
            int endIndex = endMatch.hasMatch() ? endMatch.capturedEnd() : -1;
            if (endIndex == -1) {
                setCurrentBlockState(1);
                int length = text.length() - startIndex;
                setFormat(startIndex, length, QTextCharFormat()); // we expect comment format already set by rules; otherwise leave
                break;
            } else {
                int length = endIndex - startIndex;
                QTextCharFormat comFmt = m_model ? QTextCharFormat() : QTextCharFormat();
                QTextCharFormat f = QTextCharFormat();
                f.setForeground(m_model ? m_model->colorFor(m_language, "comment", QColor("#888888")) : QColor("#888888"));
                setFormat(startIndex, length, f);
                startIndex = text.indexOf(startDelim, endIndex);
            }
        }
    }
}
