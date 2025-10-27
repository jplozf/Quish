#include "CodeEditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QMouseEvent>
#include <QDebug>
#include <QStack>
#include <QMenu>
#include <QAction>

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);
    connect(static_cast<LineNumberArea*>(lineNumberArea), &LineNumberArea::foldingIndicatorClicked, this, &CodeEditor::handleFoldingIndicatorClick);
    connect(document(), &QTextDocument::contentsChange, this, [this]() { updateFoldingRegions(); });

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    updateFoldingRegions(); // Initial scan for folding regions
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    // Add space for folding indicators
    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 15; // +15 for folding indicator

    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width() - 15, fontMetrics().height(), // Adjust width for indicator
                             Qt::AlignRight, number);

            // Draw folding indicators
            if (m_foldRegions.contains(blockNumber)) {
                painter.setPen(Qt::darkGray);
                QRect indicatorRect(lineNumberArea->width() - 15, top, 15, fontMetrics().height());
                if (m_foldedStarts.contains(blockNumber)) {
                    painter.drawText(indicatorRect, Qt::AlignCenter, "+");
                } else {
                    painter.drawText(indicatorRect, Qt::AlignCenter, "-");
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void CodeEditor::updateFoldingRegions()
{
    m_foldRegions.clear();
    QTextBlock block = document()->firstBlock();
    QStack<int> braceStack; // Stores line numbers of open braces/brackets

    int lineNumber = 0;
    while (block.isValid()) {
        QString text = block.text().trimmed();

        if (text.startsWith('{') || text.startsWith('[')) {
            braceStack.push(lineNumber);
        } else if (text.startsWith('}') || text.startsWith(']')) {
            if (!braceStack.isEmpty()) {
                int startLine = braceStack.pop();
                m_foldRegions.insert(startLine, lineNumber);
            }
        }
        block = block.next();
        lineNumber++;
    }
    // Ensure folded blocks remain folded if they still exist
    QSet<int> newFoldedStarts;
    for (int foldedLine : m_foldedStarts) {
        if (m_foldRegions.contains(foldedLine)) {
            newFoldedStarts.insert(foldedLine);
        }
    }
    m_foldedStarts = newFoldedStarts;

    // Apply folding state
    toggleFolding(-1); // -1 indicates re-applying all folding states
    lineNumberArea->update();
}

void CodeEditor::toggleFolding(int lineNumber)
{
    if (lineNumber != -1) { // -1 means re-apply all folding states
        if (m_foldRegions.contains(lineNumber)) {
            if (m_foldedStarts.contains(lineNumber)) {
                m_foldedStarts.remove(lineNumber);
            } else {
                m_foldedStarts.insert(lineNumber);
            }
        }
    }

    // Apply current folding state to all blocks
    QTextBlock block = document()->firstBlock();
    int currentFoldStart = -1;

    while (block.isValid()) {
        int currentLine = block.blockNumber();
        bool isVisible = true;

        if (m_foldRegions.contains(currentLine)) {
            if (m_foldedStarts.contains(currentLine)) {
                currentFoldStart = currentLine;
            }
        }

        if (currentFoldStart != -1) {
            if (currentLine > currentFoldStart && currentLine <= m_foldRegions.value(currentFoldStart)) {
                isVisible = false;
            }
            if (currentLine == m_foldRegions.value(currentFoldStart)) {
                currentFoldStart = -1; // End of folded block
            }
        }
        block.setVisible(isVisible);
        block = block.next();
    }
    viewport()->update();
    lineNumberArea->update();
}

QTextBlock CodeEditor::getFirstVisibleBlock() const
{
    return firstVisibleBlock();
}

void CodeEditor::handleFoldingIndicatorClick(int lineNumber)
{
    toggleFolding(lineNumber);
}

void CodeEditor::foldAll()
{
    m_foldedStarts.clear();
    for (int startLine : m_foldRegions.keys()) {
        m_foldedStarts.insert(startLine);
    }
    toggleFolding(-1);
}

void CodeEditor::unfoldAll()
{
    m_foldedStarts.clear();
    toggleFolding(-1);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    codeEditor->lineNumberAreaPaintEvent(event);
}

void LineNumberArea::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int lineNumber = codeEditor->getFirstVisibleBlock().blockNumber() + event->pos().y() / codeEditor->fontMetrics().height();
        // Check if the click is on the folding indicator area
        if (event->x() > codeEditor->lineNumberAreaWidth() - 15) {
            emit foldingIndicatorClicked(lineNumber);
        }
    } else if (event->button() == Qt::RightButton) {
        // Check if the right-click is on the folding indicator area
        if (event->x() > codeEditor->lineNumberAreaWidth() - 15) {
            QMenu menu(this);
            QAction *foldAllAction = menu.addAction(tr("Fold All"));
            QAction *unfoldAllAction = menu.addAction(tr("Unfold All"));

            connect(foldAllAction, &QAction::triggered, codeEditor, &CodeEditor::foldAll);
            connect(unfoldAllAction, &QAction::triggered, codeEditor, &CodeEditor::unfoldAll);

            menu.exec(event->globalPos());
        }
    }
    QWidget::mousePressEvent(event);
}
