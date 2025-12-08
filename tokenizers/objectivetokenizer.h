#pragma once
#include "tokenizerbase.h"

class ObjectiveCTokenizer : public TokenizerBase
{
public:
    // Tokenize Objective-C code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};