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
#pragma once

#include <QDialog>
#include <QPlainTextEdit>

namespace nwn
{

    class MdlSyntaxHighlighter;
    class LineNumberArea;

    /**
     * @brief QPlainTextEdit with line number margin.
     */
    class CodeEditor : public QPlainTextEdit
    {
        Q_OBJECT

    public:
        explicit CodeEditor(QWidget* parent = nullptr);

        void lineNumberAreaPaintEvent(QPaintEvent* event);
        int lineNumberAreaWidth();

    protected:
        void resizeEvent(QResizeEvent* event) override;

    private slots:
        void updateLineNumberAreaWidth(int newBlockCount);
        void updateLineNumberArea(const QRect& rect, int dy);

    private:
        QWidget* m_LineNumberArea;
    };

    /**
     * @brief Widget that displays line numbers for CodeEditor.
     */
    class LineNumberArea : public QWidget
    {
    public:
        explicit LineNumberArea(CodeEditor* editor) : QWidget(editor), m_CodeEditor(editor)
        {
        }

        QSize sizeHint() const override
        {
            return QSize(m_CodeEditor->lineNumberAreaWidth(), 0);
        }

    protected:
        void paintEvent(QPaintEvent* event) override
        {
            m_CodeEditor->lineNumberAreaPaintEvent(event);
        }

    private:
        CodeEditor* m_CodeEditor;
    };

    /**
     * @brief Dialog window displaying decompiled MDL ASCII source code.
     *
     * Shows the ASCII representation of a loaded model with syntax highlighting
     * for better readability. The text is read-only and scrollable.
     */
    class MdlSourceViewer : public QDialog
    {
        Q_OBJECT

    public:
        explicit MdlSourceViewer(QWidget* parent = nullptr);

        /**
         * @brief Set the MDL source text to display.
         * @param text The ASCII MDL content.
         */
        void setText(const QString& text);

        /**
         * @brief Set the model name (shown in window title).
         * @param name The model name.
         */
        void setModelName(const QString& name);

    private:
        void setupUi();

        CodeEditor* m_TextEdit;
        MdlSyntaxHighlighter* m_Highlighter;
        QString m_ModelName;
    };

} // namespace nwn
