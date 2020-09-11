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
#include "../com/model_tree.h"
#include "../com/thread_queue.h"

#include "ui_model_tree.h"

#include <src/model/object_id.h>
#include <src/storage/storage.h>

#include <QMenu>
#include <QPoint>
#include <QTreeWidget>
#include <optional>
#include <thread>
#include <unordered_map>

namespace gui
{
class ModelTree final : public QWidget, private ModelTreeEvents
{
        Q_OBJECT

private:
        const std::thread::id m_thread_id;

        Ui::ModelTree ui;

        storage::Storage m_storage;

        std::unordered_map<QTreeWidgetItem*, ObjectId> m_map_item_id;
        std::unordered_map<ObjectId, QTreeWidgetItem*> m_map_id_item;

        std::vector<Connection> m_connections;
        ThreadQueue m_thread_queue;

        void insert(storage::MeshObject&& object, const std::optional<ObjectId>& parent_object_id) override;
        void insert(storage::VolumeObject&& object, const std::optional<ObjectId>& parent_object_id) override;
        void erase(ObjectId id) override;
        void update(ObjectId id) override;
        void show(ObjectId id, bool visible) override;

        void clear();

        //void set_current(ObjectId id);

        void make_menu(const QPoint& pos);
        template <typename T>
        void make_menu_for_object(QMenu* menu, const std::shared_ptr<T>& object);

        void insert_into_tree(
                ObjectId id,
                unsigned dimension,
                const std::string& name,
                bool visible,
                const std::optional<ObjectId>& parent_object_id);
        void erase_from_tree(ObjectId id);

        void show_object(ObjectId id, bool show);
        void show_only_this_object(ObjectId id);

public:
        ModelTree();
        ~ModelTree() override;

        ModelTreeEvents* events();

        std::optional<ObjectId> current_item() const;

        std::optional<storage::MeshObject> current_mesh() const;
        std::optional<storage::MeshObjectConst> current_mesh_const() const;
        std::optional<storage::MeshObject> mesh_if_current(ObjectId id) const;
        std::optional<storage::MeshObjectConst> mesh_const_if_current(ObjectId id) const;
        std::vector<storage::MeshObjectConst> const_mesh_objects() const;

        std::optional<storage::VolumeObject> current_volume() const;
        std::optional<storage::VolumeObjectConst> current_volume_const() const;
        std::optional<storage::VolumeObject> volume_if_current(ObjectId id) const;
        std::optional<storage::VolumeObjectConst> volume_const_if_current(ObjectId id) const;
        std::vector<storage::VolumeObjectConst> const_volume_objects() const;

Q_SIGNALS:
        void item_update();
};
}
