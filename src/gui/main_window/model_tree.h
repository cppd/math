/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/model/mesh_object.h>
#include <src/model/object_id.h>
#include <src/model/volume_object.h>
#include <src/storage/storage.h>
#include <src/storage/types.h>

#include <QPoint>
#include <QTreeWidget>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace ns::gui::main_window
{
class ModelTree final : public QWidget, private ModelTreeEvents, private ModelTreeActions
{
        Q_OBJECT

private:
        struct Item final
        {
                QTreeWidgetItem* item;
                bool visible;
        };

        const std::thread::id thread_id_;

        Ui::ModelTree ui_;

        storage::Storage storage_;

        std::unordered_map<QTreeWidgetItem*, model::ObjectId> map_item_id_;
        std::unordered_map<model::ObjectId, Item> map_id_item_;

        std::vector<Connection> connections_;
        ThreadQueue thread_queue_;

        void insert(storage::MeshObject&& object, const std::optional<model::ObjectId>& parent_object_id) override;
        void insert(storage::VolumeObject&& object, const std::optional<model::ObjectId>& parent_object_id) override;
        void update(storage::MeshObjectWeak&& object) override;
        void update(storage::VolumeObjectWeak&& object) override;
        void erase(model::ObjectId id) override;

        void show(model::ObjectId id, bool show) override;
        void show_only_it(model::ObjectId id) override;
        void clear() override;

        void make_menu(const QPoint& pos);

        void insert_into_tree(
                model::ObjectId id,
                unsigned dimension,
                const std::string& name,
                const std::optional<model::ObjectId>& parent_object_id);
        void erase_from_tree(model::ObjectId id);

        template <typename T>
        void update_weak(const T& object);

        template <std::size_t N>
        void update_item(const std::shared_ptr<model::mesh::MeshObject<N>>& object);

        template <std::size_t N>
        void update_item(const std::shared_ptr<model::volume::VolumeObject<N>>& object);

public:
        ModelTree();
        ~ModelTree() override;

        ModelTreeEvents* events();

        std::optional<model::ObjectId> current_item() const;

        std::optional<storage::MeshObject> current_mesh() const;
        std::optional<storage::MeshObjectConst> current_mesh_const() const;
        std::optional<storage::MeshObject> mesh_if_current(model::ObjectId id) const;
        std::optional<storage::MeshObjectConst> mesh_const_if_current(model::ObjectId id) const;
        std::vector<storage::MeshObjectConst> const_mesh_objects() const;

        std::optional<storage::VolumeObject> current_volume() const;
        std::optional<storage::VolumeObjectConst> current_volume_const() const;
        std::optional<storage::VolumeObject> volume_if_current(model::ObjectId id) const;
        std::optional<storage::VolumeObjectConst> volume_const_if_current(model::ObjectId id) const;
        std::vector<storage::VolumeObjectConst> const_volume_objects() const;

Q_SIGNALS:
        void item_update();
};
}
