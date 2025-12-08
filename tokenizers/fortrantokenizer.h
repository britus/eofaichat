#pragma once
#include "tokenizerbase.h"

class FortranTokenizer : public TokenizerBase
{
public:
    // Tokenize Fortran code for syntax highlighting
    QVector<Token> tokenizeCode(const QString &code) override;
};