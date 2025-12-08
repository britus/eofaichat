#include "pasctokenizer.h"
#include <QColor>

QVector<Token> PascalTokenizer::tokenizeCode(const QString &code)
{
    QVector<Token> tokens;
    
    int i = 0;
    while (i < code.length()) {
        QChar c = code[i];
        
        if (c == ' ') {
            tokens.append({QString(c), "space"});
            i++;
        } else if (c == '\n') {
            tokens.append({QString(c), "newline"});
            i++;
        } else if (c == '\t') {
            tokens.append({QString(c), "tab"});
            i++;
        } else if (c == '\"') {
            // Handle strings
            QString str = QString(c);
            i++;
            while (i < code.length() && code[i] != '\"') {
                str += code[i];
                i++;
            }
            if (i < code.length()) {
                str += code[i]; // Include closing quote
                i++;
            }
            tokens.append({str, "string"});
        } else if (c == '{') {
            // Handle comments
            QString comment = "{";
            i++;
            while (i < code.length() && code[i] != '}') {
                comment += code[i];
                i++;
            }
            if (i < code.length()) {
                comment += code[i]; // Include closing brace
                i++;
            }
            tokens.append({comment, "comment"});
        } else if (c == '(' && i + 1 < code.length() && code[i + 1] == '*') {
            // Handle block comments
            QString comment = "(*";
            i += 2;
            while (i + 1 < code.length() && !(code[i] == '*' && code[i + 1] == ')')) {
                comment += code[i];
                i++;
            }
            if (i + 1 < code.length()) {
                comment += "*)";
                i += 2;
            }
            tokens.append({comment, "comment"});
        } else if (c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')') {
            tokens.append({QString(c), "bracket"});
            i++;
        } else if (c == '=' && i + 1 < code.length() && code[i + 1] == '=') {
            // Equality operator
            tokens.append({QString("=="), "operator"});
            i += 2;
        } else if (c == '<' && i + 1 < code.length() && code[i + 1] == '=') {
            // Less than or equal
            tokens.append({QString("<="), "operator"});
            i += 2;
        } else if (c == '>' && i + 1 < code.length() && code[i + 1] == '=') {
            // Greater than or equal
            tokens.append({QString(">="), "operator"});
            i += 2;
        } else if (c == '<' && i + 1 < code.length() && code[i + 1] == '>') {
            // Not equal operator
            tokens.append({QString("<>"), "operator"});
            i += 2;
        } else if (c == '+' && i + 1 < code.length() && code[i + 1] == '=') {
            // Addition assignment
            tokens.append({QString("+="), "operator"});
            i += 2;
        } else if (c == '-' && i + 1 < code.length() && code[i + 1] == '=') {
            // Subtraction assignment
            tokens.append({QString("-="), "operator"});
            i += 2;
        } else if (c == '*' && i + 1 < code.length() && code[i + 1] == '=') {
            // Multiplication assignment
            tokens.append({QString("*="), "operator"});
            i += 2;
        } else if (c == '/' && i + 1 < code.length() && code[i + 1] == '=') {
            // Division assignment
            tokens.append({QString("/="), "operator"});
            i += 2;
        } else if (c == ':' && i + 1 < code.length() && code[i + 1] == '=') {
            // Assignment operator
            tokens.append({QString(":="), "operator"});
            i += 2;
        } else if (c == '-' || c == '+' || c == '*' || c == '/' || c == '<' || c == '>' || c == '&' || c == '|' || c == '^' || c == '~' || c == '!') {
            // Single character operators
            tokens.append({QString(c), "operator"});
            i++;
        } else if (c.isDigit()) {
            // Handle numbers
            QString num = QString(c);
            i++;
            while (i < code.length() && (code[i].isDigit() || code[i] == '.')) {
                num += code[i];
                i++;
            }
            tokens.append({num, "number"});
        } else if (c.isLetter() || c == '_') {
            // Handle identifiers and keywords
            QString ident = QString(c);
            i++;
            while (i < code.length() && (code[i].isLetterOrNumber() || code[i] == '_')) {
                ident += code[i];
                i++;
            }
            
            // Check if it's a keyword
            if (ident == "program" || ident == "begin" || ident == "end" || ident == "var" || ident == "const" || ident == "type" || ident == "array" || ident == "record" || ident == "object" || ident == "class"
                || ident == "procedure" || ident == "function" || ident == "if" || ident == "then" || ident == "else" || ident == "while" || ident == "do" || ident == "for" || ident == "to" || ident == "downto" || ident == "repeat"
                || ident == "until" || ident == "case" || ident == "of" || ident == "goto" || ident == "label" || ident == "exit" || ident == "with" || ident == "uses" || ident == "implementation" || ident == "interface" || ident == "library"
                || ident == "unit" || ident == "mod" || ident == "div" || ident == "and" || ident == "or" || ident == "not" || ident == "xor" || ident == "shl" || ident == "shr" || ident == "true" || ident == "false" || ident == "nil"
                || ident == "integer" || ident == "real" || ident == "double" || ident == "char" || ident == "string" || ident == "boolean" || ident == "array" || ident == "file" || ident == "set" || ident == "packed"
                || ident == "forward" || ident == "external" || ident == "cdecl" || ident == "pascal" || ident == "register" || ident == "far" || ident == "near" || ident == "interrupt" || ident == "asm") {
                tokens.append({ident, "keyword"});
            } else {
                tokens.append({ident, "variable"});
            }
        } else {
            // Handle other characters
            tokens.append({QString(c), "variable"});
            i++;
        }
    }
    
    return tokens;
}