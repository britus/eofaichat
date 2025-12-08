#include "swifttokenizer.h"
#include <QColor>

QVector<Token> SwiftTokenizer::tokenizeCode(const QString &code)
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
        } else if (c == '\'') {
            // Handle character literals
            QString chr = QString(c);
            i++;
            while (i < code.length() && code[i] != '\'') {
                chr += code[i];
                if (code[i] == '\\') {
                    chr += code[++i]; // Include escaped character
                }
                i++;
            }
            if (i < code.length()) {
                chr += code[i]; // Include closing quote
                i++;
            }
            tokens.append({chr, "string"});
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
        } else if (c == '!' && i + 1 < code.length() && code[i + 1] == '=') {
            // Not equal operator
            tokens.append({QString("!="), "operator"});
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
        } else if (c == '-' || c == '+' || c == '*' || c == '/' || c == '%' || c == '<' || c == '>' || c == '&' || c == '|' || c == '^' || c == '~' || c == '!') {
            // Single character operators
            tokens.append({QString(c), "operator"});
            i++;
        } else if (c.isDigit()) {
            // Handle numbers
            QString num = QString(c);
            i++;
            while (i < code.length() && (code[i].isDigit() || code[i] == '.' || code[i] == 'f' || code[i] == 'F' || code[i] == 'd' || code[i] == 'D')) {
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
            if (ident == "import" || ident == "class" || ident == "struct" || ident == "enum" || ident == "protocol" || ident == "func" || ident == "var" || ident == "let" || ident == "if" || ident == "else"
                || ident == "for" || ident == "while" || ident == "do" || ident == "repeat" || ident == "switch" || ident == "case" || ident == "default" || ident == "break" || ident == "continue" || ident == "return"
                || ident == "throw" || ident == "try" || ident == "catch" || ident == "defer" || ident == "guard" || ident == "where" || ident == "as" || ident == "is" || ident == "in" || ident == "from"
                || ident == "get" || ident == "set" || ident == "willSet" || ident == "didSet" || ident == "mutating" || ident == "nonmutating" || ident == "static" || ident == "dynamic" || ident == "final"
                || ident == "override" || ident == "required" || ident == "optional" || ident == "convenience" || ident == "lazy" || ident == "public" || ident == "private" || ident == "internal"
                || ident == "fileprivate" || ident == "open" || ident == "associatedtype" || ident == "typealias" || ident == "extension" || ident == "operator" || ident == "prefix" || ident == "postfix"
                || ident == "infix" || ident == "precedence" || ident == "associativity" || ident == "left" || ident == "right" || ident == "none" || ident == "true" || ident == "false" || ident == "nil"
                || ident == "self" || ident == "super" || ident == "init" || ident == "deinit" || ident == "subscript" || ident == "protocol" || ident == "associatedtype") {
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