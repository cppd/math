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
#include <src/storage/storage.h>

#include <QTreeWidget>
#include <optional>
#include <thread>
#include <unordered_map>

namespace gui
{
class ModelTree final
{
        friend class ModelEvents;

        const std::thread::id m_thread_id;

        storage::Storage m_storage;

        std::unordered_map<QTreeWidgetItem*, ObjectId> m_map_item_id;
        std::unordered_map<ObjectId, QTreeWidgetItem*> m_map_id_item;

        QTreeWidget* m_tree = nullptr;

        std::vector<Connection> m_connections;
        std::function<void()> m_item_changed;

        //void set_current(ObjectId id);

        void make_menu(const QPoint& pos);

        void insert_into_tree_and_storage(
                const storage::MeshObject& object,
                const std::optional<ObjectId>& parent_object_id);
        void insert_into_tree_and_storage(
                const storage::VolumeObject& object,
                const std::optional<ObjectId>& parent_object_id);
        void delete_from_tree_and_storage(ObjectId id);

        void insert_into_tree(
                ObjectId id,
                unsigned dimension,
                const std::string& name,
                bool visible,
                const std::optional<ObjectId>& parent_object_id);
        void erase_from_tree(ObjectId id);

        void set_visible_in_tree(ObjectId id, bool visible);

        void show_object(ObjectId id, bool show);
        void show_only_this_object(ObjectId id);

public:
        ModelTree(QTreeWidget* tree, const std::function<void()>& item_changed);

        ~ModelTree();

        void clear();

        std::optional<ObjectId> current_item() const;

        std::optional<storage::MeshObject> current_mesh() const;
        std::optional<storage::MeshObjectConst> current_mesh_const() const;
        std::optional<storage::MeshObject> mesh_if_current(ObjectId id) const;
        std::optional<storage::MeshObjectConst> mesh_const_if_current(ObjectId id) const;

        std::optional<storage::VolumeObject> current_volume() const;
        std::optional<storage::VolumeObjectConst> current_volume_const() const;
        std::optional<storage::VolumeObject> volume_if_current(ObjectId id) const;
        std::optional<storage::VolumeObjectConst> volume_const_if_current(ObjectId id) const;
};
}
