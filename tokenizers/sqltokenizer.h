#pragma once
#include "tokenizerbase.h"

class SqlTokenizer : public TokenizerBase
{
public:
    // Tokenize SQL code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};