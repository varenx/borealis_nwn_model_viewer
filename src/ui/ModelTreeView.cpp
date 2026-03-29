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
#include "ModelTreeView.hpp"
#include <QHeaderView>

namespace nwn
{

    namespace mdl = borealis::nwn::mdl;

    ModelTreeView::ModelTreeView(QWidget* parent) : QTreeWidget(parent)
    {
        setHeaderLabels({"Name", "Type"});
        setColumnCount(2);
        header()->setSectionResizeMode(0, QHeaderView::Stretch);
        header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

        setSelectionMode(QAbstractItemView::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);

        connect(this, &QTreeWidget::itemClicked, this, &ModelTreeView::onItemClicked);
        connect(this, &QTreeWidget::itemChanged, this, &ModelTreeView::onItemChanged);
    }

    void ModelTreeView::setModel(const mdl::Model* model)
    {
        QTreeWidget::clear();

        if (!model || !model->rootNode)
        {
            return;
        }

        // Create root item for model
        auto* rootItem = new QTreeWidgetItem(this);
        rootItem->setText(0, QString::fromStdString(model->name));
        rootItem->setText(1, "Model");
        rootItem->setExpanded(true);

        // Build node tree
        buildTree(model->rootNode.get(), rootItem);

        // Add animations section if any
        if (!model->animations.empty())
        {
            auto* animsItem = new QTreeWidgetItem(rootItem);
            animsItem->setText(0, "Animations");
            animsItem->setText(1, QString::number(model->animations.size()));

            for (const auto& anim : model->animations)
            {
                auto* animItem = new QTreeWidgetItem(animsItem);
                animItem->setText(0, QString::fromStdString(anim.name));
                animItem->setText(1, QString("%1s").arg(anim.length, 0, 'f', 2));
            }
        }

        expandAll();
    }

    void ModelTreeView::clear()
    {
        QTreeWidget::clear();
    }

    void ModelTreeView::buildTree(const mdl::Node* node, QTreeWidgetItem* parentItem)
    {
        if (!node)
        {
            return;
        }

        const auto& base = node->base();

        auto* item = new QTreeWidgetItem(parentItem);
        item->setText(0, QString::fromStdString(base.name));
        item->setText(1, nodeTypeToString(node->type));
        item->setData(0, Qt::UserRole, QString::fromStdString(base.name));
        item->setData(0, Qt::UserRole + 1, static_cast<int>(node->type));
        item->setIcon(0, getNodeIcon(node->type));

        if (isMeshNode(node->type))
        {
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(0, Qt::Checked);
        }

        for (const auto& child : base.children)
        {
            buildTree(child.get(), item);
        }
    }

    QString ModelTreeView::nodeTypeToString(mdl::NodeType type)
    {
        switch (type)
        {
        case mdl::NodeType::Dummy:
            return "Dummy";
        case mdl::NodeType::Light:
            return "Light";
        case mdl::NodeType::Emitter:
            return "Emitter";
        case mdl::NodeType::Camera:
            return "Camera";
        case mdl::NodeType::Reference:
            return "Reference";
        case mdl::NodeType::TriMesh:
            return "TriMesh";
        case mdl::NodeType::SkinMesh:
            return "SkinMesh";
        case mdl::NodeType::AnimMesh:
            return "AnimMesh";
        case mdl::NodeType::DanglyMesh:
            return "DanglyMesh";
        case mdl::NodeType::AABBMesh:
            return "AABBMesh";
        default:
            return "Unknown";
        }
    }

    QIcon ModelTreeView::getNodeIcon(mdl::NodeType type) const
    {
        // Return empty icon for now - could add icons later
        Q_UNUSED(type);
        return {};
    }

    void ModelTreeView::onItemClicked(QTreeWidgetItem* item, int column)
    {
        Q_UNUSED(column);

        QString nodeName = item->data(0, Qt::UserRole).toString();
        if (!nodeName.isEmpty())
        {
            emit nodeSelected(nodeName);
        }
    }

    void ModelTreeView::onItemChanged(QTreeWidgetItem* item, int column)
    {
        if (column != 0)
        {
            return;
        }

        // Check if this is a mesh node with a checkbox
        QVariant typeData = item->data(0, Qt::UserRole + 1);
        if (!typeData.isValid())
        {
            return;
        }

        auto nodeType = static_cast<mdl::NodeType>(typeData.toInt());
        if (!isMeshNode(nodeType))
        {
            return;
        }

        QString nodeName = item->data(0, Qt::UserRole).toString();
        if (!nodeName.isEmpty())
        {
            bool visible = (item->checkState(0) == Qt::Checked);
            emit nodeVisibilityChanged(nodeName, visible);
        }
    }

    bool ModelTreeView::isMeshNode(mdl::NodeType type)
    {
        switch (type)
        {
        case mdl::NodeType::TriMesh:
        case mdl::NodeType::SkinMesh:
        case mdl::NodeType::AnimMesh:
        case mdl::NodeType::DanglyMesh:
        case mdl::NodeType::AABBMesh:
            return true;
        default:
            return false;
        }
    }

} // namespace nwn
