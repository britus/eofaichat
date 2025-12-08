#include "coboltokenizer.h"
#include <QColor>

QVector<Token> CobolTokenizer::tokenizeCode(const QString &code)
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
                i++;
            }
            if (i < code.length()) {
                str += code[i]; // Include closing quote
                i++;
            }
            tokens.append({str, "string"});
        } else if (c == '*' || c == '/') {
            // Handle comments (asterisk or slash at start of line)
            QString comment = QString(c);
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
        } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '<' || c == '>' || c == '&' || c == '|' || c == '^' || c == '~' || c == '!') {
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
            
            // Check if it's a keyword (COBOL keywords)
            if (ident == "IDENTIFICATION" || ident == "DIVISION" || ident == "PROGRAM" || ident == "ENVIRONMENT" || ident == "INPUT-OUTPUT" || ident == "FILE-CONTROL" || ident == "SELECT" || ident == "ASSIGN" || ident == "FD" || ident == "SD"
                || ident == "WORKING-STORAGE" || ident == "LOCAL-STORAGE" || ident == "FILE" || ident == "DATA" || ident == "SECTION" || ident == "PROCEDURE" || ident == "DECLARATIVES" || ident == "END-DECLARATIVES" || ident == "IF" || ident == "THEN"
                || ident == "ELSE" || ident == "ENDIF" || ident == "PERFORM" || ident == "VARYING" || ident == "FROM" || ident == "TO" || ident == "BY" || ident == "TIMES" || ident == "WHILE" || ident == "END-WHILE" || ident == "GO" || ident == "TO"
                || ident == "MOVE" || ident == "ADD" || ident == "SUBTRACT" || ident == "MULTIPLY" || ident == "DIVIDE" || ident == "COMPUTE" || ident == "SET" || ident == "RESET" || ident == "ACCEPT" || ident == "DISPLAY" || ident == "CALL"
                || ident == "RETURN" || ident == "STOP" || ident == "END" || ident == "PROGRAM" || ident == "END-PROGRAM" || ident == "END-PERFORM" || ident == "END-IF" || ident == "END-WHILE" || ident == "END-SECTION" || ident == "END-DECLARATIVES"
                || ident == "END-PROCEDURE" || ident == "END-DATA" || ident == "END-FILE" || ident == "END-ENVIRONMENT" || ident == "END-IDENTIFICATION" || ident == "TRUE" || ident == "FALSE" || ident == "ZERO" || ident == "SPACE" || ident == "NULL") {
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