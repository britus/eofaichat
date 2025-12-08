#include "typescripttokenizer.h"
#include <QColor>

QVector<Token> TypeScriptTokenizer::tokenizeCode(const QString &code)
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
        } else if (c == '\"' || c == '\'' || c == '`') {
            // Handle strings
            QChar quote = c;
            QString str = QString(c);
            i++;
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
            tokens.append({str, "string"});
        } else if (c == '/') {
            // Check for comments
            if (i + 1 < code.length()) {
                if (code[i + 1] == '*') {
                    // Block comment
                    QString comment = "/*";
                    i += 2;
                    while (i + 1 < code.length() && !(code[i] == '*' && code[i + 1] == '/')) {
                        comment += code[i];
                        i++;
                    }
                    if (i + 1 < code.length()) {
                        comment += "*/";
                        i += 2;
                    }
                    tokens.append({comment, "comment"});
                } else if (code[i + 1] == '/') {
                    // Line comment
                    QString comment = "//";
                    i += 2;
                    while (i < code.length() && code[i] != '\n') {
                        comment += code[i];
                        i++;
                    }
                    tokens.append({comment, "comment"});
                } else {
                    // Division operator
                    tokens.append({QString(c), "operator"});
                    i++;
                }
            } else {
                tokens.append({QString(c), "operator"});
                i++;
            }
        } else if (c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')') {
            tokens.append({QString(c), "bracket"});
            i++;
        } else if (c == '=' && i + 1 < code.length() && code[i + 1] == '=') {
            // Equality operator
            tokens.append({QString("=="), "operator"});
            i += 2;
        } else if (c == '=' && i + 1 < code.length() && code[i + 1] == '=') {
            // Strict equality operator
            tokens.append({QString("==="), "operator"});
            i += 2;
        } else if (c == '!' && i + 1 < code.length() && code[i + 1] == '=') {
            // Not equal operator
            tokens.append({QString("!="), "operator"});
            i += 2;
        } else if (c == '!' && i + 1 < code.length() && code[i + 1] == '=') {
            // Strict not equal operator
            tokens.append({QString("!=="), "operator"});
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
        } else if (c == '-' && i + 1 < code.length() && code[i + 1] == '-') {
            // Decrement operator
            tokens.append({QString("--"), "operator"});
            i += 2;
        } else if (c == '+' && i + 1 < code.length() && code[i + 1] == '+') {
            // Increment operator
            tokens.append({QString("++"), "operator"});
            i += 2;
        } else if (c == '-' || c == '+' || c == '*' || c == '/' || c == '%' || c == '<' || c == '>' || c == '&' || c == '|' || c == '^' || c == '~' || c == '!') {
            // Single character operators
            tokens.append({QString(c), "operator"});
            i++;
        } else if (c.isDigit()) {
            // Handle numbers
            QString num = QString(c);
            i++;
            while (i < code.length() && (code[i].isDigit() || code[i] == '.' || code[i] == 'e' || code[i] == 'E' || code[i] == 'f' || code[i] == 'F' || code[i] == 'd' || code[i] == 'D')) {
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
            if (ident == "abstract" || ident == "arguments" || ident == "await" || ident == "boolean" || ident == "break" || ident == "byte" || ident == "case" || ident == "catch" || ident == "char" || ident == "class"
                || ident == "const" || ident == "continue" || ident == "debugger" || ident == "default" || ident == "delete" || ident == "do" || ident == "double" || ident == "else" || ident == "enum" || ident == "eval"
                || ident == "export" || ident == "extends" || ident == "false" || ident == "final" || ident == "finally" || ident == "float" || ident == "for" || ident == "function" || ident == "goto" || ident == "if"
                || ident == "implements" || ident == "import" || ident == "in" || ident == "instanceof" || ident == "int" || ident == "interface" || ident == "let" || ident == "long" || ident == "native" || ident == "new"
                || ident == "null" || ident == "package" || ident == "private" || ident == "protected" || ident == "public" || ident == "return" || ident == "short" || ident == "static" || ident == "super"
                || ident == "switch" || ident == "synchronized" || ident == "this" || ident == "throw" || ident == "throws" || ident == "transient" || ident == "true" || ident == "try" || ident == "typeof" || ident == "var"
                || ident == "void" || ident == "volatile" || ident == "while" || ident == "with" || ident == "yield" || ident == "any" || ident == "as" || ident == "async" || ident == "await" || ident == "bigint" || ident == "boolean"
                || ident == "break" || ident == "case" || ident == "catch" || ident == "class" || ident == "const" || ident == "continue" || ident == "debugger" || ident == "declare" || ident == "default" || ident == "delete"
                || ident == "do" || ident == "else" || ident == "enum" || ident == "export" || ident == "extends" || ident == "false" || ident == "finally" || ident == "for" || ident == "function" || ident == "if"
                || ident == "import" || ident == "in" || ident == "instanceof" || ident == "interface" || ident == "let" || ident == "new" || ident == "null" || ident == "package" || ident == "private" || ident == "protected"
                || ident == "public" || ident == "return" || ident == "static" || ident == "super" || ident == "switch" || ident == "this" || ident == "throw" || ident == "true" || ident == "try" || ident == "typeof"
                || ident == "var" || ident == "void" || ident == "while" || ident == "with" || ident == "yield" || ident == "readonly" || ident == "unique" || ident == "symbol" || ident == "unknown" || ident == "never") {
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