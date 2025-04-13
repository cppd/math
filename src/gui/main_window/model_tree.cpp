/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "model_tree_menu.h"
#include "model_tree_style.h"

#include <src/com/error.h>
#include <src/gui/com/model_tree.h>
#include <src/model/mesh_object.h>
#include <src/model/object_id.h>
#include <src/model/volume_object.h>
#include <src/storage/types.h>

#include <QMenu>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>
#include <Qt>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

namespace ns::gui::main_window
{
ModelTree::ModelTree()
        : QWidget(nullptr),
          thread_id_(std::this_thread::get_id())
{
        ui_.setupUi(this);

        connections_.emplace_back(QObject::connect(
                ui_.model_tree, &QTreeWidget::currentItemChanged,
                [this](QTreeWidgetItem*, QTreeWidgetItem*)
                {
                        Q_EMIT item_update();
                }));

        ui_.model_tree->setContextMenuPolicy(Qt::CustomContextMenu);
        connections_.emplace_back(QObject::connect(
                ui_.model_tree, &QTreeWidget::customContextMenuRequested,
                [this](const QPoint& p)
                {
                        make_menu(p);
                }));
}

ModelTree::~ModelTree()
{
        ASSERT(std::this_thread::get_id() == thread_id_);
}

com::ModelTreeEvents* ModelTree::events()
{
        return this;
}

void ModelTree::clear()
{
        thread_queue_.push(
                [this]
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        map_item_id_.clear();
                        map_id_item_.clear();
                        storage_.clear();
                        ui_.model_tree->clear();
                });
}

template <std::size_t N>
void ModelTree::update_item(const std::shared_ptr<model::mesh::MeshObject<N>>& object)
{
        const auto iter = map_id_item_.find(object->id());
        if (iter == map_id_item_.cend())
        {
                return;
        }
        Item& item = iter->second;
        item.visible = model::mesh::Reading(*object).visible();
        set_model_tree_item_style(item.item, item.visible);
        if (item.item == ui_.model_tree->currentItem())
        {
                Q_EMIT item_update();
        }
}

template <std::size_t N>
void ModelTree::update_item(const std::shared_ptr<model::volume::VolumeObject<N>>& object)
{
        const auto iter = map_id_item_.find(object->id());
        if (iter == map_id_item_.cend())
        {
                return;
        }
        Item& item = iter->second;
        item.visible = model::volume::Reading(*object).visible();
        set_model_tree_item_style(item.item, item.visible);
        if (item.item == ui_.model_tree->currentItem())
        {
                Q_EMIT item_update();
        }
}

template <typename T>
void ModelTree::update_weak(const T& object)
{
        std::visit(
                [&](const auto& v)
                {
                        if (const auto ptr = v.lock())
                        {
                                update_item(ptr);
                        }
                },
                object);
}

void ModelTree::insert(storage::MeshObject&& object, const std::optional<model::ObjectId>& parent_object_id)
{
        thread_queue_.push(
                [this, object = std::move(object), parent_object_id]
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        std::visit(
                                [&]<std::size_t N>(const std::shared_ptr<model::mesh::MeshObject<N>>& mesh)
                                {
                                        insert_into_tree(mesh->id(), N, mesh->name(), parent_object_id);
                                        storage_.set_object(mesh);
                                        update_item(mesh);
                                },
                                object);
                });
}

void ModelTree::insert(storage::VolumeObject&& object, const std::optional<model::ObjectId>& parent_object_id)
{
        thread_queue_.push(
                [this, object = std::move(object), parent_object_id]
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        std::visit(
                                [&]<std::size_t N>(const std::shared_ptr<model::volume::VolumeObject<N>>& volume)
                                {
                                        insert_into_tree(volume->id(), N, volume->name(), parent_object_id);
                                        storage_.set_object(volume);
                                        update_item(volume);
                                },
                                object);
                });
}

void ModelTree::update(storage::MeshObjectWeak&& object)
{
        thread_queue_.push(
                [this, object = std::move(object)]
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        update_weak(object);
                });
}

void ModelTree::update(storage::VolumeObjectWeak&& object)
{
        thread_queue_.push(
                [this, object = std::move(object)]
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        update_weak(object);
                });
}

void ModelTree::erase(const model::ObjectId id)
{
        thread_queue_.push(
                [this, id]
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        storage_.delete_object(id);
                        erase_from_tree(id);
                });
}

void ModelTree::insert_into_tree(
        const model::ObjectId id,
        const unsigned dimension,
        const std::string& name,
        const std::optional<model::ObjectId>& parent_object_id)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (map_id_item_.contains(id))
        {
                return;
        }

        QTreeWidgetItem* parent_item = nullptr;
        if (parent_object_id)
        {
                const auto parent_iter = map_id_item_.find(*parent_object_id);
                if (parent_iter != map_id_item_.cend())
                {
                        parent_item = parent_iter->second.item;
                }
        }

        QTreeWidgetItem* const item = [&]
        {
                if (!parent_item)
                {
                        return new QTreeWidgetItem(ui_.model_tree);
                }
                auto* const new_item = new QTreeWidgetItem(parent_item);
                parent_item->setExpanded(true);
                return new_item;
        }();

        const QString s = QString("(%1D) %2").arg(dimension).arg(QString::fromStdString(name));
        item->setText(0, s);
        item->setToolTip(0, s);

        map_item_id_[item] = id;
        map_id_item_[id].item = item;
}

void ModelTree::erase_from_tree(const model::ObjectId id)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const auto item_iter = map_id_item_.find(id);
        if (item_iter == map_id_item_.cend())
        {
                return;
        }

        QTreeWidgetItem* item = item_iter->second.item;

        map_id_item_.erase(id);
        map_item_id_.erase(item);

        if (item->childCount() > 0)
        {
                set_model_tree_item_style_deleted(item);
                Q_EMIT item_update();
                return;
        }

        do
        {
                QTreeWidgetItem* const parent = item->parent();
                delete item;
                item = parent;
        } while (item != nullptr && item->childCount() == 0 && !map_item_id_.contains(item));
}

void ModelTree::show(const model::ObjectId id, const bool show)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const std::optional<storage::MeshObject> mesh = storage_.mesh_object(id);
        const std::optional<storage::VolumeObject> volume = storage_.volume_object(id);

        if (mesh.has_value() && volume.has_value())
        {
                error_fatal("Mesh and volume with the same id");
        }

        if (volume)
        {
                std::visit(
                        [&]<std::size_t N>(const std::shared_ptr<model::volume::VolumeObject<N>>& volume_object)
                        {
                                model::volume::Writing(volume_object.get()).set_visible(show);
                        },
                        *volume);
        }
        else if (mesh)
        {
                std::visit(
                        [&]<std::size_t N>(const std::shared_ptr<model::mesh::MeshObject<N>>& mesh_object)
                        {
                                model::mesh::Writing(mesh_object.get()).set_visible(show);
                        },
                        *mesh);
        }
}

void ModelTree::show_only_it(const model::ObjectId id)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        for (const auto& v : map_id_item_)
        {
                if (id != v.first)
                {
                        show(v.first, false);
                }
        }
        show(id, true);
}

std::optional<model::ObjectId> ModelTree::current_item() const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const auto iter = map_item_id_.find(ui_.model_tree->currentItem());
        if (iter != map_item_id_.cend())
        {
                return iter->second;
        }
        return std::nullopt;
}

std::optional<storage::MeshObject> ModelTree::current_mesh() const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (const auto id = current_item())
        {
                return storage_.mesh_object(*id);
        }
        return std::nullopt;
}

std::optional<storage::MeshObjectConst> ModelTree::current_mesh_const() const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (const auto id = current_item())
        {
                return storage_.mesh_object_const(*id);
        }
        return std::nullopt;
}

std::optional<storage::MeshObjectConst> ModelTree::mesh_const_if_current(const model::ObjectId id) const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (const auto current_id = current_item(); id == current_id)
        {
                return storage_.mesh_object_const(id);
        }
        return std::nullopt;
}

std::optional<storage::MeshObject> ModelTree::mesh_if_current(const model::ObjectId id) const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (const auto current_id = current_item(); id == current_id)
        {
                return storage_.mesh_object(id);
        }
        return std::nullopt;
}

std::optional<storage::VolumeObject> ModelTree::current_volume() const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (const auto id = current_item())
        {
                return storage_.volume_object(*id);
        }
        return std::nullopt;
}

std::optional<storage::VolumeObjectConst> ModelTree::current_volume_const() const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (const auto id = current_item())
        {
                return storage_.volume_object_const(*id);
        }
        return std::nullopt;
}

std::optional<storage::VolumeObjectConst> ModelTree::volume_const_if_current(const model::ObjectId id) const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (const auto current_id = current_item(); id == current_id)
        {
                return storage_.volume_object_const(id);
        }
        return std::nullopt;
}

std::optional<storage::VolumeObject> ModelTree::volume_if_current(const model::ObjectId id) const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (const auto current_id = current_item(); id == current_id)
        {
                return storage_.volume_object(id);
        }
        return std::nullopt;
}

std::vector<storage::MeshObjectConst> ModelTree::const_mesh_objects() const
{
        return storage_.objects<storage::MeshObjectConst>();
}

std::vector<storage::VolumeObjectConst> ModelTree::const_volume_objects() const
{
        return storage_.objects<storage::VolumeObjectConst>();
}

void ModelTree::make_menu(const QPoint& pos)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        QTreeWidgetItem* const item = ui_.model_tree->itemAt(pos);
        if (item != ui_.model_tree->currentItem())
        {
                return;
        }

        const auto id_iter = map_item_id_.find(item);
        if (id_iter == map_item_id_.cend())
        {
                return;
        }

        const auto item_iter = map_id_item_.find(id_iter->second);
        ASSERT(item_iter != map_id_item_.cend());

        const std::unique_ptr<QMenu> menu =
                make_model_tree_menu_for_object(this, id_iter->second, item_iter->second.visible);

        if (menu)
        {
                menu->exec(ui_.model_tree->mapToGlobal(pos));
        }
}
}
