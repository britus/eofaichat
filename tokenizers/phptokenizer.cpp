#include "phptokenizer.h"
#include <QColor>

QVector<Token> PhpTokenizer::tokenizeCode(const QString &code)
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
        } else if (c == '-' || c == '+' || c == '*' || c == '/' || c == '%' || c == '<' || c == '>' || c == '&' || c == '|' || c == '^' || c == '~' || c == '!') {
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
            if (ident == "echo" || ident == "print" || ident == "include" || ident == "include_once" || ident == "require" || ident == "require_once" || ident == "isset" || ident == "unset" || ident == "empty"
                || ident == "array" || ident == "true" || ident == "false" || ident == "null" || ident == "var" || ident == "const" || ident == "global" || ident == "static" || ident == "public" || ident == "private"
                || ident == "protected" || ident == "abstract" || ident == "final" || ident == "class" || ident == "interface" || ident == "trait" || ident == "function" || ident == "if" || ident == "else"
                || ident == "elseif" || ident == "for" || ident == "foreach" || ident == "while" || ident == "do" || ident == "switch" || ident == "case" || ident == "default" || ident == "break" || ident == "continue"
                || ident == "return" || ident == "goto" || ident == "try" || ident == "catch" || ident == "finally" || ident == "throw" || ident == "new" || ident == "extends" || ident == "implements" || ident == "use"
                || ident == "namespace" || ident == "yield" || ident == "list" || ident == "array" || ident == "as" || ident == "default" || ident == "die" || ident == "exit" || ident == "eval" || ident == "call_user_func"
                || ident == "call_user_func_array" || ident == "create_function" || ident == "eval" || ident == "assert" || ident == "compact" || ident == "extract" || ident == "parse_str" || ident == "unserialize"
                || ident == "serialize" || ident == "var_dump" || ident == "debug_zval_dump" || ident == "print_r" || ident == "var_export" || ident == "debug_print_backtrace" || ident == "debug_backtrace"
                || ident == "class_exists" || ident == "function_exists" || ident == "method_exists" || ident == "property_exists" || ident == "interface_exists" || ident == "trait_exists" || ident == "class_implements"
                || ident == "class_parents" || ident == "get_class" || ident == "get_parent_class" || ident == "is_a" || ident == "is_subclass_of" || ident == "is_object" || ident == "is_array" || ident == "is_bool"
                || ident == "is_callable" || ident == "is_double" || ident == "is_float" || ident == "is_int" || ident == "is_integer" || ident == "is_long" || ident == "is_null" || ident == "is_numeric" || ident == "is_real"
                || ident == "is_resource" || ident == "is_scalar" || ident == "is_string" || ident == "is_uploaded_file" || ident == "is_writable" || ident == "is_writeable" || ident == "is_dir" || ident == "is_executable"
                || ident == "is_file" || ident == "is_link" || ident == "is_readable" || ident == "is_uploaded_file" || ident == "is_writeable" || ident == "is_writable" || ident == "file_exists" || ident == "file_get_contents"
                || ident == "file_put_contents" || ident == "fopen" || ident == "fclose" || ident == "fgets" || ident == "fread" || ident == "fwrite" || ident == "fputs" || ident == "feof" || ident == "ferror" || ident == "fflush"
                || ident == "fseek" || ident == "ftell" || ident == "rewind" || ident == "ftruncate" || ident == "file" || ident == "filemtime" || ident == "filectime" || ident == "filesize" || ident == "fileatime"
                || ident == "fileinode" || ident == "fileowner" || ident == "filegroup" || ident == "fileperms" || ident == "filetype" || ident == "is_uploaded_file" || ident == "move_uploaded_file" || ident == "basename"
                || ident == "dirname" || ident == "pathinfo" || ident == "realpath" || ident == "parse_url" || ident == "http_build_query" || ident == "parse_ini_file" || ident == "parse_ini_string" || ident == "parse_str"
                || ident == "parse_url" || ident == "http_build_query" || ident == "parse_ini_file" || ident == "parse_ini_string" || ident == "parse_str") {
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