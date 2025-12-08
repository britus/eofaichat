#pragma once
#include <tokenizerbase.h>
#include <QMap>
#include <QString>
#include <QVector>

class ChatTextTokenizer
{
public:
    // Tokenize code for syntax highlighting
    static QVector<Token> tokenizeCode(const QString &code, const QString &language);
    // Convert tokens to HTML with syntax highlighting
    static QString tokensToHtml(const QVector<Token> &tokens, const QString &language, const class SyntaxColorModel *model);
    // Create a map from file extensions to language types
    static QString fileExtToLanguage(const QString &extension);
};
