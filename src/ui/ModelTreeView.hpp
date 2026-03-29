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

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <borealis/nwn/mdl/Mdl.hpp>

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    class ModelTreeView : public QTreeWidget
    {
        Q_OBJECT

    public:
        explicit ModelTreeView(QWidget* parent = nullptr);

        // Populate tree from model
        void setModel(const mdl::Model* model);
        void clear();

    signals:
        void nodeSelected(const QString& nodeName);
        void nodeVisibilityChanged(const QString& nodeName, bool visible);

    private:
        void buildTree(const mdl::Node* node, QTreeWidgetItem* parentItem);
        static QString nodeTypeToString(mdl::NodeType type);
        QIcon getNodeIcon(mdl::NodeType type) const;

    private slots:
        void onItemClicked(QTreeWidgetItem* item, int column);
        void onItemChanged(QTreeWidgetItem* item, int column);

    private:
        static bool isMeshNode(mdl::NodeType type);
    };

} // namespace nwn
