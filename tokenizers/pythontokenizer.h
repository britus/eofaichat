#pragma once
#include "tokenizerbase.h"

class PythonTokenizer : public TokenizerBase
{
public:
    // Tokenize Python code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};