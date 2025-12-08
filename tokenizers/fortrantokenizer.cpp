#include "fortrantokenizer.h"
#include <QColor>

QVector<Token> FortranTokenizer::tokenizeCode(const QString &code)
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
        } else if (c == '!') {
            // Handle comments
            QString comment = "!";
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
        } else if (c == '/' && i + 1 < code.length() && code[i + 1] == '=') {
            // Not equal operator
            tokens.append({QString("/="), "operator"});
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
        } else if (c == '*' || c == '/' || c == '+' || c == '-' || c == '<' || c == '>' || c == '&' || c == '|' || c == '^' || c == '~' || c == '!') {
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
            if (ident == "PROGRAM" || ident == "SUBROUTINE" || ident == "FUNCTION" || ident == "END" || ident == "IF" || ident == "THEN" || ident == "ELSE" || ident == "ENDIF" || ident == "DO" || ident == "END DO"
                || ident == "WHILE" || ident == "END WHILE" || ident == "FOR" || ident == "END FOR" || ident == "CALL" || ident == "RETURN" || ident == "CONTINUE" || ident == "STOP" || ident == "GOTO" || ident == "ASSIGN"
                || ident == "OPEN" || ident == "CLOSE" || ident == "READ" || ident == "WRITE" || ident == "PRINT" || ident == "FORMAT" || ident == "DIMENSION" || ident == "INTEGER" || ident == "REAL" || ident == "DOUBLE"
                || ident == "COMPLEX" || ident == "LOGICAL" || ident == "CHARACTER" || ident == "DATA" || ident == "PARAMETER" || ident == "COMMON" || ident == "SAVE" || ident == "EXTERNAL" || ident == "INTRINSIC"
                || ident == "PUBLIC" || ident == "PRIVATE" || ident == "ALLOCATABLE" || ident == "ALLOCATE" || ident == "DEALLOCATE" || ident == "NULLIFY" || ident == "POINTER" || ident == "TARGET" || ident == "CONTIGUOUS"
                || ident == "VOLATILE" || ident == "PROCEDURE" || ident == "MODULE" || ident == "END MODULE" || ident == "USE" || ident == "IMPLICIT" || ident == "INCLUDE" || ident == "LABEL" || ident == "ASSIGNMENT"
                || ident == "OPERATOR" || ident == "INTERFACE" || ident == "END INTERFACE" || ident == "TYPE" || ident == "END TYPE" || ident == "STRUCTURE" || ident == "END STRUCTURE" || ident == "ENUM" || ident == "END ENUM"
                || ident == "SELECT" || ident == "CASE" || ident == "DEFAULT" || ident == "END SELECT" || ident == "WHERE" || ident == "ELSE WHERE" || ident == "END WHERE" || ident == "SYNC" || ident == "ALL" || ident == "ANY"
                || ident == "TRUE" || ident == "FALSE") {
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