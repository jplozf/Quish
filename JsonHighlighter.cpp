#include "JsonHighlighter.h"
#include <QDebug>

JsonHighlighter::JsonHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // Keywords (keys in JSON objects)
    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp("\\\"[a-zA-Z_][a-zA-Z0-9_]*\\\"(?=\\s*:)"); // Simple keys
    rule.format = keywordFormat;
    highlightingRules.append(rule);

    // String values
    valueStringFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("(?<=\\:\\s*)\\\"(\\\\.|[^\\\"\\])*\\\""); // Matches "value" after a colon
    rule.format = valueStringFormat;
    highlightingRules.append(rule);

    // Number values
    valueNumberFormat.setForeground(Qt::darkMagenta);
    rule.pattern = QRegExp("(?<=\\:\\s*)\\b[0-9]+\\b"); // Simple numbers
    rule.format = valueNumberFormat;
    highlightingRules.append(rule);

    // Boolean values
    booleanFormat.setForeground(Qt::blue);
    booleanFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp("(?<=\\:\\s*)\\b(true|false)\\b"); // Matches true/false after a colon
    rule.format = booleanFormat;
    highlightingRules.append(rule);

    // Null value
    nullFormat.setForeground(Qt::gray);
    nullFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegExp("(?<=\\:\\s*)\\bnull\\b"); // Matches null after a colon
    rule.format = nullFormat;
    highlightingRules.append(rule);

    // Generic strings (not keys or values) - e.g., within arrays
    QTextCharFormat genericStringFormat;
    genericStringFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("\\\"(\\\\.|[^\\\"\\])*\\\""); // Matches any string
    rule.format = genericStringFormat;
    highlightingRules.append(rule);
}

void JsonHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }
}
