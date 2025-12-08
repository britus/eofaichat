#pragma once
#include "tokenizerbase.h"

class CppTokenizer : public TokenizerBase
{
public:
    // Tokenize C++ code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};