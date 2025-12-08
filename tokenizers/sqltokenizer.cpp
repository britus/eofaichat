#include "sqltokenizer.h"
#include <QColor>

QVector<Token> SqlTokenizer::tokenizeCode(const QString &code)
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
        } else if (c == '-') {
            // Check for comments
            if (i + 1 < code.length() && code[i + 1] == '-') {
                // Line comment
                QString comment = "--";
                i += 2;
                while (i < code.length() && code[i] != '\n') {
                    comment += code[i];
                    i++;
                }
                tokens.append({comment, "comment"});
            } else {
                // Minus operator
                tokens.append({QString(c), "operator"});
                i++;
            }
        } else if (c == '/' && i + 1 < code.length() && code[i + 1] == '*') {
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
        } else if (c == '<' && i + 1 < code.length() && code[i + 1] == '>') {
            // Not equal operator (alternative)
            tokens.append({QString("<>"), "operator"});
            i += 2;
        } else if (c == '<' || c == '>' || c == '&' || c == '|' || c == '^' || c == '~' || c == '!') {
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
            if (ident == "SELECT" || ident == "FROM" || ident == "WHERE" || ident == "INSERT" || ident == "UPDATE" || ident == "DELETE" || ident == "CREATE" || ident == "DROP" || ident == "ALTER" || ident == "TABLE"
                || ident == "INDEX" || ident == "VIEW" || ident == "PROCEDURE" || ident == "FUNCTION" || ident == "TRIGGER" || ident == "DATABASE" || ident == "SCHEMA" || ident == "PRIMARY" || ident == "FOREIGN" || ident == "KEY"
                || ident == "CONSTRAINT" || ident == "UNIQUE" || ident == "NOT" || ident == "NULL" || ident == "DEFAULT" || ident == "AUTO_INCREMENT" || ident == "INCREMENT" || ident == "VALUES" || ident == "INTO" || ident == "SET"
                || ident == "AS" || ident == "JOIN" || ident == "INNER" || ident == "LEFT" || ident == "RIGHT" || ident == "OUTER" || ident == "ON" || ident == "GROUP" || ident == "BY" || ident == "HAVING" || ident == "ORDER" || ident == "ASC"
                || ident == "DESC" || ident == "LIMIT" || ident == "OFFSET" || ident == "DISTINCT" || ident == "BETWEEN" || ident == "LIKE" || ident == "IN" || ident == "EXISTS" || ident == "ALL" || ident == "ANY" || ident == "UNION"
                || ident == "INTERSECT" || ident == "EXCEPT" || ident == "CASE" || ident == "WHEN" || ident == "THEN" || ident == "ELSE" || ident == "END" || ident == "BEGIN" || ident == "END" || ident == "IF" || ident == "ELSE"
                || ident == "FOR" || ident == "WHILE" || ident == "DO" || ident == "LOOP" || ident == "DECLARE" || ident == "BEGIN" || ident == "COMMIT" || ident == "ROLLBACK" || ident == "SAVEPOINT" || ident == "TRANSACTION"
                || ident == "WITH" || ident == "AS" || ident == "USING" || ident == "ORDER" || ident == "FETCH" || ident == "FIRST" || ident == "NEXT" || ident == "ROWS" || ident == "ONLY" || ident == "FOR" || ident == "UPDATE"
                || ident == "SHARE" || ident == "LOCK" || ident == "OF" || ident == "NOWAIT" || ident == "WAIT" || ident == "SKIP" || ident == "UNLOCK" || ident == "REPLACE" || ident == "TEMPORARY" || ident == "TEMP" || ident == "IF"
                || ident == "NOT" || ident == "EXISTS" || ident == "OR" || ident == "AND" || ident == "TRUE" || ident == "FALSE") {
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