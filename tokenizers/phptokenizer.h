#pragma once
#include "tokenizerbase.h"

class PhpTokenizer : public TokenizerBase
{
public:
    // Tokenize PHP code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};