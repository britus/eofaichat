#pragma once
#include <QVector>
#include <QString>

// Token structure for syntax highlighting
struct Token
{
    QString text;
    QString type; // keyword, comment, string, number, variable, bracket, space, tab, newline, preprocessor, operator
};

class TokenizerBase
{
public:
    // Virtual destructor for proper inheritance
    virtual ~TokenizerBase() = default;
    
    // Pure virtual function for tokenizing code
    virtual QVector<Token> tokenizeCode(const QString &code) = 0;
    
    // Convert tokens to HTML with syntax highlighting
    static QString tokensToHtml(const QVector<Token> &tokens, const QString &language, const class SyntaxColorModel *model);
};