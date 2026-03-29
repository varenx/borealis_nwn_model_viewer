/*
 * This file is part of Borealis NWN Model Viewer.
 * Copyright (C) 2025-2026 Varenx
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "MdlSourceViewer.hpp"
#include <QFont>
#include <QFontDatabase>
#include <QPainter>
#include <QTextBlock>
#include <QVBoxLayout>
#include "MdlSyntaxHighlighter.hpp"

namespace nwn
{

    // CodeEditor implementation

    CodeEditor::CodeEditor(QWidget* parent) : QPlainTextEdit(parent)
    {
        m_LineNumberArea = new LineNumberArea(this);

        connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
        connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);

        updateLineNumberAreaWidth(0);
    }

    int CodeEditor::lineNumberAreaWidth()
    {
        int digits = 1;
        int max = qMax(1, blockCount());
        while (max >= 10)
        {
            max /= 10;
            ++digits;
        }
        // Minimum 4 digits width for consistency
        digits = qMax(digits, 4);

        int space = 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
        return space;
    }

    void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
    {
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    }

    void CodeEditor::updateLineNumberArea(const QRect& rect, int dy)
    {
        if (dy)
        {
            m_LineNumberArea->scroll(0, dy);
        }
        else
        {
            m_LineNumberArea->update(0, rect.y(), m_LineNumberArea->width(), rect.height());
        }

        if (rect.contains(viewport()->rect()))
        {
            updateLineNumberAreaWidth(0);
        }
    }

    void CodeEditor::resizeEvent(QResizeEvent* event)
    {
        QPlainTextEdit::resizeEvent(event);

        QRect cr = contentsRect();
        m_LineNumberArea->setGeometry(
            QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    }

    void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event)
    {
        QPainter painter(m_LineNumberArea);

        // Gray background
        painter.fillRect(event->rect(), QColor(230, 230, 230));

        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        int bottom = top + qRound(blockBoundingRect(block).height());

        // Black text color
        painter.setPen(Qt::black);

        while (block.isValid() && top <= event->rect().bottom())
        {
            if (block.isVisible() && bottom >= event->rect().top())
            {
                QString number = QString::number(blockNumber + 1);
                painter.drawText(0, top, m_LineNumberArea->width() - 4, fontMetrics().height(),
                                 Qt::AlignRight, number);
            }

            block = block.next();
            top = bottom;
            bottom = top + qRound(blockBoundingRect(block).height());
            ++blockNumber;
        }
    }

    // MdlSourceViewer implementation

    MdlSourceViewer::MdlSourceViewer(QWidget* parent) : QDialog(parent)
    {
        setupUi();
    }

    void MdlSourceViewer::setupUi()
    {
        setWindowTitle("MDL Source");
        resize(900, 700);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 4, 4, 4);

        m_TextEdit = new CodeEditor(this);
        m_TextEdit->setReadOnly(true);
        m_TextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

        // Use monospace font
        QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setPointSize(10);
        m_TextEdit->setFont(font);

        // Set tab width to 4 spaces
        QFontMetrics metrics(font);
        m_TextEdit->setTabStopDistance(4 * metrics.horizontalAdvance(' '));

        layout->addWidget(m_TextEdit);

        // Create syntax highlighter
        m_Highlighter = new MdlSyntaxHighlighter(m_TextEdit->document());
    }

    void MdlSourceViewer::setText(const QString& text)
    {
        m_TextEdit->setPlainText(text);
        // Move cursor to beginning
        m_TextEdit->moveCursor(QTextCursor::Start);
    }

    void MdlSourceViewer::setModelName(const QString& name)
    {
        m_ModelName = name;
        if (name.isEmpty())
        {
            setWindowTitle("MDL Source");
        }
        else
        {
            setWindowTitle(QString("MDL Source - %1").arg(name));
        }
    }

} // namespace nwn
