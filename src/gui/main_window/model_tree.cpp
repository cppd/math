/*
Copyright (C) 2017-2020 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "model_tree.h"

#include <src/com/error.h>

#include <memory>

namespace gui
{
ModelTree::ModelTree(QTreeWidget* tree) : m_tree(tree)
{
        ASSERT(m_tree);

        connect(m_tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this,
                SLOT(current_item_changed(QTreeWidgetItem*, QTreeWidgetItem*)));
}

ModelTree::~ModelTree()
{
        clear_all();
}

void ModelTree::clear_all()
{
        QSignalBlocker blocker(m_tree);
        m_tree->clear();
        m_map_item_id.clear();
        m_map_id_item.clear();
}

void ModelTree::add_item(ObjectId id, unsigned dimension, const std::string& name)
{
        auto iter = m_map_id_item.find(id);
        if (iter != m_map_id_item.cend())
        {
                return;
        }
        std::unique_ptr<QTreeWidgetItem> item = std::make_unique<QTreeWidgetItem>();
        QString s = QString("(%1D) %2").arg(dimension).arg(QString::fromStdString(name));
        item->setText(0, s);
        item->setToolTip(0, s);
        QTreeWidgetItem* ptr = item.get();
        m_tree->addTopLevelItem(item.release());
        m_map_item_id[ptr] = id;
        m_map_id_item[id] = ptr;
}

void ModelTree::delete_item(ObjectId id)
{
        auto iter = m_map_id_item.find(id);
        if (iter == m_map_id_item.cend())
        {
                return;
        }
        int index = m_tree->indexOfTopLevelItem(iter->second);
        ASSERT(index >= 0);
        if (index >= 0)
        {
                delete m_tree->takeTopLevelItem(index);
        }
        m_map_id_item.erase(iter->first);
        m_map_item_id.erase(iter->second);
}

std::optional<ObjectId> ModelTree::current_item() const
{
        auto iter = m_map_item_id.find(m_tree->currentItem());
        if (iter != m_map_item_id.cend())
        {
                return iter->second;
        }
        return std::nullopt;
}

void ModelTree::set_current(ObjectId id)
{
        auto iter = m_map_id_item.find(id);
        if (iter == m_map_id_item.cend())
        {
                return;
        }
        m_tree->setCurrentItem(iter->second);
}

void ModelTree::current_item_changed(QTreeWidgetItem* /*current*/, QTreeWidgetItem* /*previous*/)
{
        emit item_changed();
}
}
