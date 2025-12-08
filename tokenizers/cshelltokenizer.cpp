#include "cshelltokenizer.h"
#include <QColor>

QVector<Token> CShellTokenizer::tokenizeCode(const QString &code)
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
            if (ident == "if" || ident == "then" || ident == "else" || ident == "elif" || ident == "fi" || ident == "for" || ident == "while" || ident == "do" || ident == "done" || ident == "case"
                || ident == "esac" || ident == "function" || ident == "local" || ident == "return" || ident == "break" || ident == "continue" || ident == "export" || ident == "declare" || ident == "typeset"
                || ident == "readonly" || ident == "eval" || ident == "exec" || ident == "cd" || ident == "pwd" || ident == "ls" || ident == "cat" || ident == "echo" || ident == "read" || ident == "grep"
                || ident == "find" || ident == "sed" || ident == "awk" || ident == "cut" || ident == "sort" || ident == "uniq" || ident == "wc" || ident == "head" || ident == "tail" || ident == "chmod"
                || ident == "chown" || ident == "mkdir" || ident == "rm" || ident == "cp" || ident == "mv" || ident == "ln" || ident == "ps" || ident == "kill" || ident == "ping" || ident == "ssh"
                || ident == "scp" || ident == "git" || ident == "sudo" || ident == "apt" || ident == "yum" || ident == "pip" || ident == "python" || ident == "perl" || ident == "ruby" || ident == "php"
                || ident == "set" || ident == "unset" || ident == "shift" || ident == "trap" || ident == "umask" || ident == "alias" || ident == "unalias" || ident == "bg" || ident == "fg" || ident == "jobs"
                || ident == "kill" || ident == "wait" || ident == "time" || ident == "history" || ident == "fc" || ident == "hash" || ident == "type" || ident == "which" || ident == "whereis"
                || ident == "help" || ident == "man" || ident == "info" || ident == "less" || ident == "more" || ident == "cat" || ident == "tac" || ident == "rev" || ident == "tr" || ident == "fold"
                || ident == "column" || ident == "join" || ident == "paste" || ident == "split" || ident == "diff" || ident == "patch" || ident == "cmp" || ident == "md5sum" || ident == "sha1sum"
                || ident == "gzip" || ident == "gunzip" || ident == "bzip2" || ident == "tar" || ident == "zip" || ident == "unzip" || ident == "dd" || ident == "cp" || ident == "mv" || ident == "rm"
                || ident == "mkdir" || ident == "rmdir" || ident == "chmod" || ident == "chown" || ident == "chgrp" || ident == "ln" || ident == "ls" || ident == "find" || ident == "locate"
                || ident == "whereis" || ident == "which" || ident == "type" || ident == "test" || ident == "expr" || ident == "let" || ident == "bc" || ident == "date" || ident == "cal"
                || ident == "uptime" || ident == "who" || ident == "w" || ident == "users" || ident == "last" || ident == "finger" || ident == "passwd" || ident == "su" || ident == "sudo"
                || ident == "adduser" || ident == "deluser" || ident == "groupadd" || ident == "groupdel" || ident == "useradd" || ident == "userdel" || ident == "chpasswd" || ident == "chsh"
                || ident == "crontab" || ident == "at" || ident == "atq" || ident == "atrm" || ident == "cron" || ident == "service" || ident == "systemctl" || ident == "chkconfig" || ident == "init"
                || ident == "runlevel" || ident == "telinit" || ident == "reboot" || ident == "shutdown" || ident == "halt" || ident == "poweroff" || ident == "killall" || ident == "pkill"
                || ident == "kill" || ident == "ps" || ident == "top" || ident == "htop" || ident == "vmstat" || ident == "iostat" || ident == "sar" || ident == "netstat" || ident == "ss"
                || ident == "ifconfig" || ident == "ping" || ident == "traceroute" || ident == "mtr" || ident == "nslookup" || ident == "dig" || ident == "host" || ident == "wget" || ident == "curl"
                || ident == "ftp" || ident == "scp" || ident == "sftp" || ident == "ssh" || ident == "telnet" || ident == "nc" || ident == "nmap" || ident == "tcpdump" || ident == "wireshark"
                || ident == "iptables" || ident == "firewall-cmd" || ident == "ufw" || ident == "selinux" || ident == "apparmor" || ident == "auditd" || ident == "logrotate" || ident == "rsyslog"
                || ident == "syslog" || ident == "journalctl" || ident == "dmesg" || ident == "tail" || ident == "head" || ident == "less" || ident == "more" || ident == "cat" || ident == "grep"
                || ident == "find" || ident == "locate" || ident == "whereis" || ident == "which" || ident == "type" || ident == "test" || ident == "expr" || ident == "let" || ident == "bc"
                || ident == "date" || ident == "cal" || ident == "uptime" || ident == "who" || ident == "w" || ident == "users" || ident == "last" || ident == "finger" || ident == "passwd"
                || ident == "su" || ident == "sudo" || ident == "adduser" || ident == "deluser" || ident == "groupadd" || ident == "groupdel" || ident == "useradd" || ident == "userdel" || ident == "chpasswd"
                || ident == "chsh" || ident == "crontab" || ident == "at" || ident == "atq" || ident == "atrm" || ident == "cron" || ident == "service" || ident == "systemctl" || ident == "chkconfig"
                || ident == "init" || ident == "runlevel" || ident == "telinit" || ident == "reboot" || ident == "shutdown" || ident == "halt" || ident == "poweroff" || ident == "killall" || ident == "pkill") {
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