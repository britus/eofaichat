#pragma once
#include "tokenizerbase.h"

class CobolTokenizer : public TokenizerBase
{
public:
    // Tokenize COBOL code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};