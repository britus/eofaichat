#pragma once
#include "syntaxcolormodel.h"
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

// Ein einfacher, regex-basierter Highlighter, der anhand der Sprache Patterns verwendet.
// Für Produktivcode kannst du die Patterns je Sprache weiter detaillieren.
class CodeHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    CodeHighlighter(QTextDocument *parent, const QString &language, SyntaxColorModel *model);

protected:
    void highlightBlock(const QString &text) override;

private:
    void setupRulesForLanguage(const QString &language);

    struct Rule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<Rule> rules;
    QTextCharFormat defaultFormat;
    QString m_language;
    SyntaxColorModel *m_model;
};
