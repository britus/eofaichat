#pragma once
#include "tokenizerbase.h"

class SwiftTokenizer : public TokenizerBase
{
public:
    // Tokenize Swift code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};