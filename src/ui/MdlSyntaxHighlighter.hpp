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

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

namespace nwn
{

    /**
     * @brief Syntax highlighter for MDL ASCII format.
     *
     * Highlights MDL keywords, node types, properties, comments, and numbers
     * with distinct colors for better readability.
     */
    class MdlSyntaxHighlighter : public QSyntaxHighlighter
    {
        Q_OBJECT

    public:
        explicit MdlSyntaxHighlighter(QTextDocument* parent = nullptr);

    protected:
        void highlightBlock(const QString& text) override;

    private:
        struct HighlightingRule
        {
            QRegularExpression pattern;
            QTextCharFormat format;
        };

        void setupRules();

        QVector<HighlightingRule> m_Rules;

        // Formats for different categories
        QTextCharFormat m_StructureFormat; // Structure keywords (blue)
        QTextCharFormat m_NodeTypeFormat; // Node types (purple)
        QTextCharFormat m_PropertyFormat; // Properties (dark cyan)
        QTextCharFormat m_CommentFormat; // Comments (green)
        QTextCharFormat m_NumberFormat; // Numbers (dark red)
    };

} // namespace nwn
