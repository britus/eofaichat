#pragma once
#include "tokenizerbase.h"

class CShellTokenizer : public TokenizerBase
{
public:
    // Tokenize C Shell code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};