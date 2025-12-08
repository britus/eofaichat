#include <bashtokenizer.h>
#include <chattexttokenizer.h>
#include <coboltokenizer.h>
#include <cpptokenizer.h>
#include <cshelltokenizer.h>
#include <fortrantokenizer.h>
#include <javascripttokenizer.h>
#include <javatokenizer.h>
#include <objectivetokenizer.h>
#include <pasctokenizer.h>
#include <phptokenizer.h>
#include <pythontokenizer.h>
#include <sapabaptokenizer.h>
#include <sqltokenizer.h>
#include <swifttokenizer.h>
#include <syntaxcolormodel.h>
#include <typescripttokenizer.h>
#include <QColor>
#include <QDebug>
#include <QMap>
#include <QString>

QVector<Token> ChatTextTokenizer::tokenizeCode(const QString &code, const QString &language)
{
    QVector<Token> tokens;

    qDebug() << "[SRCCTT] tokenizeCode language:" << language;

    // For supported languages, use specific tokenizers
    if (!language.isEmpty()) {
        if (language == "cpp" || language == "c" || language == "h" || language == "hpp") {
            CppTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "java") {
            JavaTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "javascript" || language == "js") {
            JavaScriptTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "sql") {
            SqlTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "typescript" || language == "ts") {
            TypeScriptTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "python" || language == "py") {
            PythonTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "bash") {
            BashTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "pascal" || language == "pas") {
            PascalTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "sapabap") {
            SapAbapTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "fortran" || language == "f") {
            FortranTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "cobol") {
            CobolTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "objective-c" || language == "m" || language == "mm") {
            ObjectiveCTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "swift") {
            SwiftTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "php") {
            PhpTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        } else if (language == "csh") {
            CShellTokenizer tokenizer;
            return tokenizer.tokenizeCode(code);
        }
    }

    // Fallback to basic tokenization if language is not recognized
    // Simple tokenization for minimal coloring - strings, brackets, parentheses, numbers
    int i = 0;
    while (i < code.length()) {
        QChar c = code[i];

        if (c == '\n') {
            tokens.append({QString(c), "newline"});
            i++;
        } else if (c == ' ') {
            tokens.append({QString(c), "space"});
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
        } else if (c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')') {
            tokens.append({QString(c), "bracket"});
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
        } else {
            // Handle identifiers and keywords
            QString ident = QString(c);
            i++;
            while (i < code.length() && (code[i].isLetterOrNumber() || code[i] == '_')) {
                ident += code[i];
                i++;
            }
            tokens.append({ident, "variable"});
        }
    }

    return tokens;
}

QString ChatTextTokenizer::tokensToHtml(const QVector<Token> &tokens, const QString &language, const SyntaxColorModel *model)
{
    return TokenizerBase::tokensToHtml(tokens, language, model);
}

static QMap<QString, QString> extensionToLanguage;

// Create a map from file extensions to language types
QString ChatTextTokenizer::fileExtToLanguage(const QString &extension)
{
    QString ext = extension;
    if (ext.isEmpty())
        return "";

    if (extensionToLanguage.isEmpty()) {
        // C++ files
        extensionToLanguage["cpp"] = "cpp";
        extensionToLanguage["cc"] = "cpp";
        extensionToLanguage["cxx"] = "cpp";
        extensionToLanguage["c++"] = "cpp";
        extensionToLanguage["hpp"] = "cpp";
        extensionToLanguage["hxx"] = "cpp";
        extensionToLanguage["h++"] = "cpp";

        // C files
        extensionToLanguage["c"] = "c";
        extensionToLanguage["h"] = "c";

        // Java files
        extensionToLanguage["java"] = "java";

        // JavaScript files
        extensionToLanguage["js"] = "js";
        extensionToLanguage["mjs"] = "js";
        extensionToLanguage["cjs"] = "js";

        // TypeScript files
        extensionToLanguage["ts"] = "ts";
        extensionToLanguage["tsx"] = "ts";

        // SQL files
        extensionToLanguage["sql"] = "sql";

        // Bash files
        extensionToLanguage["sh"] = "bash";
        extensionToLanguage["bash"] = "bash";

        // Python files
        extensionToLanguage["py"] = "python";
        extensionToLanguage["py3"] = "python";

        // Go files
        extensionToLanguage["go"] = "go";

        // Rust files
        extensionToLanguage["rs"] = "rust";

        // PHP files
        extensionToLanguage["php"] = "php";

        // Ruby files
        extensionToLanguage["rb"] = "ruby";

        // HTML files
        extensionToLanguage["html"] = "html";
        extensionToLanguage["htm"] = "html";

        // CSS files
        extensionToLanguage["css"] = "css";

        // JSON files
        extensionToLanguage["json"] = "json";

        // YAML files
        extensionToLanguage["yml"] = "yaml";
        extensionToLanguage["yaml"] = "yaml";

        // XML files
        extensionToLanguage["xml"] = "xml";

        // Perl files
        extensionToLanguage["pl"] = "perl";
        extensionToLanguage["pm"] = "perl";

        // Scala files
        extensionToLanguage["scala"] = "scala";

        // Swift files
        extensionToLanguage["swift"] = "swift";

        // Kotlin files
        extensionToLanguage["kt"] = "kotlin";
        extensionToLanguage["kts"] = "kotlin";

        // R files
        extensionToLanguage["r"] = "r";

        // MATLAB files
        extensionToLanguage["m"] = "matlab";

        // Lua files
        extensionToLanguage["lua"] = "lua";

        // Haskell files
        extensionToLanguage["hs"] = "haskell";

        // Erlang files
        extensionToLanguage["erl"] = "erlang";
        extensionToLanguage["hrl"] = "erlang";

        // Clojure files
        extensionToLanguage["clj"] = "clojure";
        extensionToLanguage["cljs"] = "clojure";

        // Groovy files
        extensionToLanguage["groovy"] = "groovy";
        extensionToLanguage["gvy"] = "groovy";

        // Dart files
        extensionToLanguage["dart"] = "dart";

        // F# files
        extensionToLanguage["fs"] = "fsharp";
        extensionToLanguage["fsi"] = "fsharp";
        extensionToLanguage["fsx"] = "fsharp";

        // OCaml files
        extensionToLanguage["ml"] = "ocaml";
        extensionToLanguage["mli"] = "ocaml";

        // Pascal files
        extensionToLanguage["pas"] = "pascal";

        // Fortran files
        extensionToLanguage["f"] = "fortran";
        extensionToLanguage["f90"] = "fortran";
        extensionToLanguage["f95"] = "fortran";
        extensionToLanguage["f03"] = "fortran";

        // COBOL files
        extensionToLanguage["cbl"] = "cobol";
        extensionToLanguage["cob"] = "cobol";

        // Assembly files
        extensionToLanguage["asm"] = "assembly";
        extensionToLanguage["s"] = "assembly";

        // Lisp files
        extensionToLanguage["lisp"] = "lisp";
        extensionToLanguage["lsp"] = "lisp";

        // Scheme files
        extensionToLanguage["scm"] = "scheme";
        extensionToLanguage["ss"] = "scheme";

        // Tcl files
        extensionToLanguage["tcl"] = "tcl";

        // Racket files
        extensionToLanguage["rkt"] = "racket";

        // PowerShell files
        extensionToLanguage["ps1"] = "powershell";
        extensionToLanguage["psm1"] = "powershell";

        // Shell files
        extensionToLanguage["sh"] = "shell";
        extensionToLanguage["zsh"] = "shell";

        // Makefile
        extensionToLanguage["makefile"] = "makefile";
        extensionToLanguage["Makefile"] = "makefile";

        // Dockerfile
        extensionToLanguage["Dockerfile"] = "dockerfile";
    }

    if (ext.startsWith('.')) {
        ext = ext.mid(1);
    }

    if (!extensionToLanguage.contains(ext)) {
        return "";
    }

    return extensionToLanguage[ext];
}
