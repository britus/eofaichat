#pragma once
#include "tokenizerbase.h"

class PascalTokenizer : public TokenizerBase
{
public:
    // Tokenize Pascal code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};