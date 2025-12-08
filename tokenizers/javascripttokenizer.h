#pragma once
#include "tokenizerbase.h"

class JavaScriptTokenizer : public TokenizerBase
{
public:
    // Tokenize JavaScript code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};