#pragma once
#include "tokenizerbase.h"

class BashTokenizer : public TokenizerBase
{
public:
    // Tokenize Bash code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};