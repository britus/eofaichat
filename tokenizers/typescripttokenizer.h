#pragma once
#include "tokenizerbase.h"

class TypeScriptTokenizer : public TokenizerBase
{
public:
    // Tokenize TypeScript code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};