#include "pythontokenizer.h"
#include <QColor>

QVector<Token> PythonTokenizer::tokenizeCode(const QString &code)
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
        } else if (c == '\"' || c == '\'') {
            // Handle strings (including triple quotes)
            QChar quote = c;
            QString str = QString(c);
            i++;
            
            // Check for triple quotes
            if (i + 1 < code.length() && code[i] == quote && code[i+1] == quote) {
                str += code[i];
                str += code[i+1];
                i += 2;
                while (i + 2 < code.length() && !(code[i] == quote && code[i+1] == quote && code[i+2] == quote)) {
                    str += code[i];
                    i++;
                }
                if (i + 2 < code.length()) {
                    str += code[i];
                    str += code[i+1];
                    str += code[i+2];
                    i += 3;
                }
            } else {
                // Regular quotes
                while (i < code.length() && code[i] != quote) {
                    str += code[i];
                    if (code[i] == '\\') {
                        str += code[++i]; // Include escaped character
                    }
                    i++;
                }
                if (i < code.length()) {
                    str += code[i]; // Include closing quote
                    i++;
                }
            }
            tokens.append({str, "string"});
        } else if (c == '#') {
            // Handle comments
            QString comment = "#";
            i++;
            while (i < code.length() && code[i] != '\n') {
                comment += code[i];
                i++;
            }
            tokens.append({comment, "comment"});
        } else if (c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')') {
            tokens.append({QString(c), "bracket"});
            i++;
        } else if (c == '=' && i + 1 < code.length() && code[i + 1] == '=') {
            // Equality operator
            tokens.append({QString("=="), "operator"});
            i += 2;
        } else if (c == '!' && i + 1 < code.length() && code[i + 1] == '=') {
            // Not equal operator
            tokens.append({QString("!="), "operator"});
            i += 2;
        } else if (c == '<' && i + 1 < code.length() && code[i + 1] == '=') {
            // Less than or equal
            tokens.append({QString("<="), "operator"});
            i += 2;
        } else if (c == '>' && i + 1 < code.length() && code[i + 1] == '=') {
            // Greater than or equal
            tokens.append({QString(">="), "operator"});
            i += 2;
        } else if (c == '<' && i + 1 < code.length() && code[i + 1] == '<') {
            // Left shift
            tokens.append({QString("<<"), "operator"});
            i += 2;
        } else if (c == '>' && i + 1 < code.length() && code[i + 1] == '>') {
            // Right shift
            tokens.append({QString(">>"), "operator"});
            i += 2;
        } else if (c == '&' && i + 1 < code.length() && code[i + 1] == '&') {
            // Logical AND operator
            tokens.append({QString("&&"), "operator"});
            i += 2;
        } else if (c == '|' && i + 1 < code.length() && code[i + 1] == '|') {
            // Logical OR operator
            tokens.append({QString("||"), "operator"});
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
        } else if (c == '%' && i + 1 < code.length() && code[i + 1] == '=') {
            // Modulo assignment
            tokens.append({QString("%="), "operator"});
            i += 2;
        } else if (c == '/' && i + 1 < code.length() && code[i + 1] == '/') {
            // Floor division
            tokens.append({QString("//"), "operator"});
            i += 2;
        } else if (c == '-' || c == '+' || c == '*' || c == '/' || c == '%' || c == '<' || c == '>' || c == '&' || c == '|' || c == '^' || c == '~' || c == '!') {
            // Single character operators
            tokens.append({QString(c), "operator"});
            i++;
        } else if (c.isDigit()) {
            // Handle numbers
            QString num = QString(c);
            i++;
            while (i < code.length() && (code[i].isDigit() || code[i] == '.' || code[i] == 'e' || code[i] == 'E')) {
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
            if (ident == "and" || ident == "as" || ident == "assert" || ident == "async" || ident == "await" || ident == "break" || ident == "class" || ident == "continue" || ident == "def" || ident == "del"
                || ident == "elif" || ident == "else" || ident == "except" || ident == "False" || ident == "finally" || ident == "for" || ident == "from" || ident == "global" || ident == "if" || ident == "import"
                || ident == "in" || ident == "is" || ident == "lambda" || ident == "None" || ident == "nonlocal" || ident == "not" || ident == "or" || ident == "pass" || ident == "raise" || ident == "return"
                || ident == "True" || ident == "try" || ident == "while" || ident == "with" || ident == "yield") {
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