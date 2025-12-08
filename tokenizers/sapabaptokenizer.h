#pragma once
#include "tokenizerbase.h"

class SapAbapTokenizer : public TokenizerBase
{
public:
    // Tokenize SAP ABAP code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};