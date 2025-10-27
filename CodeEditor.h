#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QWidget>
#include <QMap>
#include <QSet>

class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;

class LineNumberArea;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

    void updateFoldingRegions();
    void toggleFolding(int lineNumber);

    QTextBlock getFirstVisibleBlock() const; // Public wrapper for firstVisibleBlock()

public slots:
    void foldAll();
    void unfoldAll();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void handleFoldingIndicatorClick(int lineNumber);

private:
    QWidget *lineNumberArea;
    QMap<int, int> m_foldRegions; // Map: startLine -> endLine
    QSet<int> m_foldedStarts;     // Set of startLines that are currently folded
};

class LineNumberArea : public QWidget
{
    Q_OBJECT
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor)
    {}

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

signals:
    void foldingIndicatorClicked(int lineNumber);

private:
    CodeEditor *codeEditor;
};

#endif
