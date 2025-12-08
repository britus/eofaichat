#include "objectivetokenizer.h"
#include <QColor>

QVector<Token> ObjectiveCTokenizer::tokenizeCode(const QString &code)
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
            if (ident == "int" || ident == "float" || ident == "double" || ident == "char" || ident == "bool" || ident == "void" || ident == "if"
                || ident == "else" || ident == "for" || ident == "while" || ident == "return" || ident == "class" || ident == "struct"
                || ident == "slots" || ident == "signals" || ident == "enum" || ident == "namespace" || ident == "template" || ident == "const"
                || ident == "static" || ident == "constexpr" || ident == "auto" || ident == "extern" || ident == "register" || ident == "typedef"
                || ident == "unsigned" || ident == "signed" || ident == "short" || ident == "long" || ident == "volatile" || ident == "inline"
                || ident == "virtual" || ident == "public" || ident == "private" || ident == "protected" || ident == "friend" || ident == "throw"
                || ident == "try" || ident == "catch" || ident == "switch" || ident == "case" || ident == "default" || ident == "continue"
                || ident == "break" || ident == "sizeof" || ident == "new" || ident == "delete" || ident == "this" || ident == "goto"
                || ident == "@interface" || ident == "@implementation" || ident == "@end" || ident == "@protocol" || ident == "@property"
                || ident == "@synthesize" || ident == "@dynamic" || ident == "@autoreleasepool" || ident == "@try" || ident == "@catch"
                || ident == "@finally" || ident == "@throw" || ident == "@selector" || ident == "@class" || ident == "@import"
                || ident == "id" || ident == "IBOutlet" || ident == "IBAction" || ident == "nonatomic" || ident == "strong" || ident == "weak"
                || ident == "assign" || ident == "copy" || ident == "retain" || ident == "readonly" || ident == "readwrite") {
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