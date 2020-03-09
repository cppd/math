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
#include <src/com/log.h>

#include <memory>

ModelTree::ModelTree(QWidget* parent) : QWidget(parent)
{
        ui.setupUi(this);

        connect(ui.tree_widget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this,
                SLOT(current_item_changed(QTreeWidgetItem*, QTreeWidgetItem*)));
}

void ModelTree::add_item(int id, const std::string& name)
{
        auto iter = m_map_id_item.find(id);
        if (iter != m_map_id_item.cend())
        {
                return;
        }
        std::unique_ptr<QTreeWidgetItem> item = std::make_unique<QTreeWidgetItem>();
        item->setText(0, name.c_str());
        QTreeWidgetItem* ptr = item.get();
        ui.tree_widget->addTopLevelItem(item.release());
        m_map_item_id[ptr] = id;
        m_map_id_item[id] = ptr;
}

void ModelTree::delete_item(int id)
{
        auto iter = m_map_id_item.find(id);
        if (iter == m_map_id_item.cend())
        {
                return;
        }
        int index = ui.tree_widget->indexOfTopLevelItem(iter->second);
        ASSERT(index >= 0);
        if (index >= 0)
        {
                delete ui.tree_widget->takeTopLevelItem(index);
        }
        m_map_id_item.erase(iter->first);
        m_map_item_id.erase(iter->second);
}

void ModelTree::delete_all()
{
        for (const auto& [id, item] : m_map_id_item)
        {
                int index = ui.tree_widget->indexOfTopLevelItem(item);
                ASSERT(index >= 0);
                if (index >= 0)
                {
                        delete ui.tree_widget->takeTopLevelItem(index);
                }
        }
        ASSERT(ui.tree_widget->topLevelItemCount() == 0);
        m_map_item_id.clear();
        m_map_id_item.clear();
}

std::optional<int> ModelTree::current_item() const
{
        auto iter = m_map_item_id.find(ui.tree_widget->currentItem());
        if (iter != m_map_item_id.cend())
        {
                return iter->second;
        }
        return std::nullopt;
}

void ModelTree::set_current(int id)
{
        auto iter = m_map_id_item.find(id);
        if (iter == m_map_id_item.cend())
        {
                return;
        }
        ui.tree_widget->setCurrentItem(iter->second);
}

void ModelTree::current_item_changed(QTreeWidgetItem* /*current*/, QTreeWidgetItem* /*previous*/)
{
        emit item_changed();
}
