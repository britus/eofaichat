#include "tokenizerbase.h"
#include <syntaxcolormodel.h>
#include <QColor>

QString TokenizerBase::tokensToHtml(const QVector<Token> &tokens, const QString &language, const SyntaxColorModel *model)
{
    QString html = "";

    for (const Token &token : tokens) {
        QColor color = Qt::white;

        QString text = token.text.toHtmlEscaped();
        text = text.replace('\n', "<br>");
        text = text.replace('\t', "&nbsp;&nbsp;&nbsp;&nbsp;");

        // Handle newlines in tokens
        if (token.type == "newline") {
            html += "<br>";
        } else if (token.type == "space") {
            html += "&nbsp;";
        } else if (token.type == "tab") {
            html += "&nbsp;&nbsp;&nbsp;&nbsp;";
        } else {
            // Apply fallback coloring if language is not determined or model is null
            if (language.isEmpty() || language == "system" || !model) {
                if (token.type == "string") {
                    color = QColor(100, 200, 100); // Green for strings
                } else if (token.type == "bracket") {
                    color = QColor(200, 100, 200); // Purple for brackets
                } else if (token.type == "number") {
                    color = QColor(200, 200, 100); // Yellow for numbers
                } else if (token.type == "newline") {
                    color = Qt::white; // Newline doesn't need special coloring
                } else if (token.type == "preprocessor") {
                    color = QColor(255, 100, 100); // Red for preprocessor directives
                } else if (token.type == "operator") {
                    color = QColor(200, 200, 200); // Gray for operators
                } else if (token.type == "keyword") {
                    color = QColor(100, 150, 255); // Blue for keywords
                } else {
                    color = Qt::gray;
                }
            } else if (model && model->hasLanguage(language)) {
                color = model->colorFor(language, token.type, Qt::white);
            }
            html += QString("<span style=\"font-family: Consolas,monospace,'Menlo','Courier New'; font-size: 16pt;color: %1;\">%2</span>").arg(color.name(), text);
        }
    }

    return html;
}
