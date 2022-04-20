/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "model_tree_actions.h"

#include "../com/connection.h"
#include "../com/model_tree.h"
#include "../com/thread_queue.h"

#include "ui_model_tree.h"

#include <src/model/object_id.h>
#include <src/storage/storage.h>

#include <QPoint>
#include <QTreeWidget>
#include <optional>
#include <thread>
#include <unordered_map>

namespace ns::gui::main_window
{
class ModelTree final : public QWidget, private ModelTreeEvents, private ModelTreeActions
{
        Q_OBJECT

private:
        const std::thread::id thread_id_;

        Ui::ModelTree ui_;

        storage::Storage storage_;

        struct Item final
        {
                QTreeWidgetItem* item;
                bool visible;
        };
        std::unordered_map<QTreeWidgetItem*, ObjectId> map_item_id_;
        std::unordered_map<ObjectId, Item> map_id_item_;

        std::vector<Connection> connections_;
        ThreadQueue thread_queue_;

        void insert(storage::MeshObject&& object, const std::optional<ObjectId>& parent_object_id) override;
        void insert(storage::VolumeObject&& object, const std::optional<ObjectId>& parent_object_id) override;
        void update(storage::MeshObjectWeak&& object) override;
        void update(storage::VolumeObjectWeak&& object) override;
        void erase(ObjectId id) override;

        void show(ObjectId id, bool show) override;
        void show_only_it(ObjectId id) override;
        void clear() override;

        void make_menu(const QPoint& pos);

        void insert_into_tree(
                ObjectId id,
                unsigned dimension,
                const std::string& name,
                const std::optional<ObjectId>& parent_object_id);
        void erase_from_tree(ObjectId id);

        template <typename T>
        void update_weak(const T& object);

        template <std::size_t N>
        void update_item(const std::shared_ptr<mesh::MeshObject<N>>& object);

        template <std::size_t N>
        void update_item(const std::shared_ptr<volume::VolumeObject<N>>& object);

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
