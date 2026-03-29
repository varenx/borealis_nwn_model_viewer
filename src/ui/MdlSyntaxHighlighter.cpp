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
#include "MdlSyntaxHighlighter.hpp"

namespace nwn
{

    MdlSyntaxHighlighter::MdlSyntaxHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent)
    {
        setupRules();
    }

    void MdlSyntaxHighlighter::setupRules()
    {
        // Structure keywords (blue)
        m_StructureFormat.setForeground(QColor(0, 0, 180));
        m_StructureFormat.setFontWeight(QFont::Bold);

        // Node types (purple)
        m_NodeTypeFormat.setForeground(QColor(128, 0, 128));
        m_NodeTypeFormat.setFontWeight(QFont::Bold);

        // Properties (dark cyan)
        m_PropertyFormat.setForeground(QColor(0, 128, 128));

        // Comments (green)
        m_CommentFormat.setForeground(QColor(0, 128, 0));
        m_CommentFormat.setFontItalic(true);

        // Numbers (dark red)
        m_NumberFormat.setForeground(QColor(160, 0, 0));

        // Structure keywords
        QStringList structureKeywords = {
            "\\b#MAXMODEL\\b",         "\\b#MAXGEOM\\b",       "\\bnewmodel\\b",
            "\\bdonemodel\\b",         "\\bbeginmodelgeom\\b", "\\bendmodelgeom\\b",
            "\\bnewanim\\b",           "\\bdoneanim\\b",       "\\bnode\\b",
            "\\bendnode\\b",           "\\bsetsupermodel\\b",  "\\bclassification\\b",
            "\\bsetanimationscale\\b", "\\bfiledependency\\b", "\\bfiledependancy\\b",
            "\\bignorefog\\b"};

        for (const QString& pattern : structureKeywords)
        {
            HighlightingRule rule;
            rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
            rule.format = m_StructureFormat;
            m_Rules.append(rule);
        }

        // Node types
        QStringList nodeTypes = {
            "\\bdummy\\b", "\\btrimesh\\b", "\\bskin\\b",    "\\banimmesh\\b",  "\\bdanglymesh\\b",
            "\\baabb\\b",  "\\blight\\b",   "\\bemitter\\b", "\\breference\\b", "\\bcamera\\b"};

        for (const QString& pattern : nodeTypes)
        {
            HighlightingRule rule;
            rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
            rule.format = m_NodeTypeFormat;
            m_Rules.append(rule);
        }

        // Properties - common node properties
        QStringList properties = {
            "\\bparent\\b", "\\bposition\\b", "\\borientation\\b", "\\bscale\\b",
            "\\binheritcolor\\b",
            // Mesh properties
            "\\bbitmap\\b", "\\bdiffuse\\b", "\\bambient\\b", "\\bspecular\\b", "\\bshininess\\b",
            "\\bverts\\b", "\\btverts\\b", "\\btverts1\\b", "\\bfaces\\b", "\\bcolors\\b",
            "\\brender\\b", "\\bshadow\\b", "\\btransparencyhint\\b", "\\btilefade\\b",
            "\\bmaterialname\\b", "\\bbeaming\\b", "\\brotatetexture\\b",
            // Skin mesh
            "\\bweights\\b",
            // Dangly mesh
            "\\bconstraints\\b", "\\bdisplacement\\b", "\\btightness\\b", "\\bperiod\\b",
            // Anim mesh
            "\\bsampleperiod\\b", "\\banimverts\\b", "\\banimtverts\\b",
            // Light
            "\\bcolor\\b", "\\bradius\\b", "\\bmultiplier\\b", "\\bflareradius\\b",
            "\\blightpriority\\b", "\\bambientonly\\b", "\\bndynamictype\\b", "\\baffectdynamic\\b",
            "\\blensflares\\b", "\\bfadinglight\\b", "\\bflaresizes\\b", "\\bflarepositions\\b",
            "\\bflarecolorshifts\\b", "\\btexturenames\\b",
            // Reference
            "\\brefmodel\\b", "\\breattachable\\b",
            // Emitter
            "\\bupdate\\b", "\\bblend\\b", "\\btexture\\b", "\\bchunkname\\b", "\\bxgrid\\b",
            "\\bygrid\\b", "\\bspawntype\\b", "\\btwosidedtex\\b", "\\bloop\\b",
            "\\brenderorder\\b", "\\bbirthrate\\b", "\\blifeexp\\b", "\\bvelocity\\b",
            "\\brandvel\\b", "\\bspread\\b", "\\bparticlerot\\b", "\\bmass\\b", "\\bcolorstart\\b",
            "\\bcolormid\\b", "\\bcolorend\\b", "\\balphastart\\b", "\\balphamid\\b",
            "\\balphaend\\b", "\\bsizestart\\b", "\\bsizemid\\b", "\\bsizeend\\b",
            "\\bsizestart_y\\b", "\\bsizemid_y\\b", "\\bsizeend_y\\b", "\\bpercentstart\\b",
            "\\bpercentmid\\b", "\\bpercentend\\b", "\\bxsize\\b", "\\bysize\\b", "\\bfps\\b",
            "\\bframestart\\b", "\\bframeend\\b", "\\bbounce_co\\b", "\\bgrav\\b", "\\bdrag\\b",
            "\\bthreshold\\b", "\\bp2p_bezier2\\b", "\\bp2p_bezier3\\b", "\\bcombinetime\\b",
            "\\blightningdelay\\b", "\\blightningradius\\b", "\\blightningscale\\b",
            "\\bdeadspace\\b", "\\bblastradius\\b", "\\bblastlength\\b", "\\bblurlength\\b",
            "\\bp2p\\b", "\\bp2p_sel\\b", "\\baffectedbywind\\b", "\\bm_istinted\\b",
            "\\bbounce\\b", "\\brandom\\b", "\\binherit\\b", "\\binheritvel\\b",
            "\\binherit_local\\b", "\\bsplat\\b", "\\binherit_part\\b",
            // Animation
            "\\blength\\b", "\\btranstime\\b", "\\banimroot\\b", "\\bevent\\b", "\\bpositionkey\\b",
            "\\borientationkey\\b", "\\bscalekey\\b", "\\bbirthratekey\\b", "\\blifeexpkey\\b",
            "\\bvelocitykey\\b", "\\balphastartkey\\b", "\\balphamidkey\\b", "\\balphaendkey\\b",
            "\\bsizestartkey\\b", "\\bsizemidkey\\b", "\\bsizeendkey\\b", "\\bcolorstartkey\\b",
            "\\bcolormidkey\\b", "\\bcolorendkey\\b"};

        for (const QString& pattern : properties)
        {
            HighlightingRule rule;
            rule.pattern = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
            rule.format = m_PropertyFormat;
            m_Rules.append(rule);
        }

        // Numbers (integers and floats, including negative)
        // Excludes numbers that are part of identifiers (e.g., "TTR01_S07_01.max")
        HighlightingRule numberRule;
        numberRule.pattern =
            QRegularExpression("(?<![a-zA-Z_\\d])-?\\d+\\.?\\d*(?:[eE][+-]?\\d+)?(?![a-zA-Z_])");
        numberRule.format = m_NumberFormat;
        m_Rules.append(numberRule);

        // Comments (# to end of line) - must be last to override others
        HighlightingRule commentRule;
        commentRule.pattern = QRegularExpression("#.*$");
        commentRule.format = m_CommentFormat;
        m_Rules.append(commentRule);
    }

    void MdlSyntaxHighlighter::highlightBlock(const QString& text)
    {
        for (const HighlightingRule& rule : m_Rules)
        {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext())
            {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }

} // namespace nwn
