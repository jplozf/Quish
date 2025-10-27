#ifndef JSONHIGHLIGHTER_H
#define JSONHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class JsonHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    JsonHighlighter(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat valueStringFormat;
    QTextCharFormat valueNumberFormat;
    QTextCharFormat booleanFormat;
    QTextCharFormat nullFormat;
};

#endif // JSONHIGHLIGHTER_H
