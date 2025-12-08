#pragma once
#include "tokenizerbase.h"

class JavaTokenizer : public TokenizerBase
{
public:
    // Tokenize Java code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};