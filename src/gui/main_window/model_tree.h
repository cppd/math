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

#pragma once

#include "../com/connection.h"

#include <src/model/object_id.h>

#include <QTreeWidget>
#include <unordered_map>

namespace gui
{
class ModelTree final
{
public:
        explicit ModelTree(QTreeWidget* tree, const std::function<void()>& item_changed);

        ~ModelTree();

        void add_item(ObjectId id, unsigned dimension, const std::string& name);
        void delete_item(ObjectId id);
        std::optional<ObjectId> current_item() const;
        void set_current(ObjectId id);
        void clear_all();

private:
        std::unordered_map<QTreeWidgetItem*, ObjectId> m_map_item_id;
        std::unordered_map<ObjectId, QTreeWidgetItem*> m_map_id_item;

        QTreeWidget* m_tree = nullptr;

        std::vector<Connection> m_connections;
};
}
