/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "../dialogs/message.h"

#include <src/com/error.h>

#include <QMenu>
#include <memory>

namespace ns::gui::main_window
{
namespace
{
constexpr QColor COLOR_VISIBLE(0, 0, 0);
constexpr QColor COLOR_HIDDEN(128, 128, 128);

void set_item_color(QTreeWidgetItem* const item, const bool visible)
{
        item->setForeground(0, visible ? COLOR_VISIBLE : COLOR_HIDDEN);
}
}

ModelTree::ModelTree() : QWidget(nullptr), thread_id_(std::this_thread::get_id())
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

ModelTreeEvents* ModelTree::events()
{
        return this;
}

void ModelTree::clear()
{
        thread_queue_.push(
                [this]()
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        map_item_id_.clear();
                        map_id_item_.clear();
                        storage_.clear();
                        ui_.model_tree->clear();
                });
}

template <std::size_t N>
void ModelTree::update_item(const std::shared_ptr<mesh::MeshObject<N>>& object)
{
        const auto iter = map_id_item_.find(object->id());
        if (iter == map_id_item_.cend())
        {
                return;
        }
        Item& item = iter->second;
        item.visible = mesh::Reading(*object).visible();
        set_item_color(item.item, item.visible);
        if (item.item == ui_.model_tree->currentItem())
        {
                Q_EMIT item_update();
        }
}

template <std::size_t N>
void ModelTree::update_item(const std::shared_ptr<volume::VolumeObject<N>>& object)
{
        const auto iter = map_id_item_.find(object->id());
        if (iter == map_id_item_.cend())
        {
                return;
        }
        Item& item = iter->second;
        item.visible = volume::Reading(*object).visible();
        set_item_color(item.item, item.visible);
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

void ModelTree::insert(storage::MeshObject&& object, const std::optional<ObjectId>& parent_object_id)
{
        thread_queue_.push(
                [this, object = std::move(object), parent_object_id]()
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        std::visit(
                                [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& mesh)
                                {
                                        insert_into_tree(mesh->id(), N, mesh->name(), parent_object_id);
                                        storage_.set_object(mesh);
                                        update_item(mesh);
                                },
                                object);
                });
}

void ModelTree::insert(storage::VolumeObject&& object, const std::optional<ObjectId>& parent_object_id)
{
        thread_queue_.push(
                [this, object = std::move(object), parent_object_id]()
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        std::visit(
                                [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume)
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
                [this, object = std::move(object)]()
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        update_weak(object);
                });
}

void ModelTree::update(storage::VolumeObjectWeak&& object)
{
        thread_queue_.push(
                [this, object = std::move(object)]()
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        update_weak(object);
                });
}

void ModelTree::erase(const ObjectId id)
{
        thread_queue_.push(
                [this, id]()
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        storage_.delete_object(id);
                        erase_from_tree(id);
                });
}

void ModelTree::insert_into_tree(
        const ObjectId id,
        const unsigned dimension,
        const std::string& name,
        const std::optional<ObjectId>& parent_object_id)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (map_id_item_.contains(id))
        {
                return;
        }

        QTreeWidgetItem* parent_item = nullptr;
        if (parent_object_id)
        {
                auto parent_iter = map_id_item_.find(*parent_object_id);
                if (parent_iter != map_id_item_.cend())
                {
                        parent_item = parent_iter->second.item;
                }
        }

        QTreeWidgetItem* item;
        if (parent_item)
        {
                item = new QTreeWidgetItem(parent_item);
                parent_item->setExpanded(true);
        }
        else
        {
                item = new QTreeWidgetItem(ui_.model_tree);
        }

        const QString s = QString("(%1D) %2").arg(dimension).arg(QString::fromStdString(name));
        item->setText(0, s);
        item->setToolTip(0, s);

        map_item_id_[item] = id;
        map_id_item_[id].item = item;
}

void ModelTree::erase_from_tree(const ObjectId id)
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
                QFont font = item->font(0);
                font.setStrikeOut(true);
                item->setFont(0, font);
                set_item_color(item, false);
                Q_EMIT item_update();
        }
        else
        {
                do
                {
                        QTreeWidgetItem* parent = item->parent();
                        delete item;
                        item = parent;
                } while (item != nullptr && item->childCount() == 0 && map_item_id_.count(item) == 0);
        }
}

void ModelTree::show_object(const ObjectId id, const bool show)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::optional<storage::MeshObject> m = storage_.mesh_object(id);
        std::optional<storage::VolumeObject> v = storage_.volume_object(id);

        if (m.has_value() && v.has_value())
        {
                error_fatal("Mesh and volume with the same id");
        }

        if (v)
        {
                std::visit(
                        [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object)
                        {
                                set_visible(volume_object.get(), show);
                        },
                        *v);
        }
        else if (m)
        {
                std::visit(
                        [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& mesh_object)
                        {
                                set_visible(mesh_object.get(), show);
                        },
                        *m);
        }
}

void ModelTree::show_only_this_object(const ObjectId id)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        for (const auto& v : map_id_item_)
        {
                if (id != v.first)
                {
                        show_object(v.first, false);
                }
        }
        show_object(id, true);
}

std::optional<ObjectId> ModelTree::current_item() const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        auto iter = map_item_id_.find(ui_.model_tree->currentItem());
        if (iter != map_item_id_.cend())
        {
                return iter->second;
        }
        return std::nullopt;
}

std::optional<storage::MeshObject> ModelTree::current_mesh() const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::optional<ObjectId> id = current_item();
        if (!id)
        {
                return std::nullopt;
        }
        std::optional<storage::MeshObject> object = storage_.mesh_object(*id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::MeshObjectConst> ModelTree::current_mesh_const() const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::optional<ObjectId> id = current_item();
        if (!id)
        {
                return std::nullopt;
        }
        std::optional<storage::MeshObjectConst> object = storage_.mesh_object_const(*id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::MeshObjectConst> ModelTree::mesh_const_if_current(const ObjectId id) const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::optional<ObjectId> current_id = current_item();
        if (!current_id || id != current_id)
        {
                return std::nullopt;
        }
        std::optional<storage::MeshObjectConst> object = storage_.mesh_object_const(id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::MeshObject> ModelTree::mesh_if_current(const ObjectId id) const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::optional<ObjectId> current_id = current_item();
        if (!current_id || id != current_id)
        {
                return std::nullopt;
        }
        std::optional<storage::MeshObject> object = storage_.mesh_object(id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::VolumeObject> ModelTree::current_volume() const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::optional<ObjectId> id = current_item();
        if (!id)
        {
                return std::nullopt;
        }
        std::optional<storage::VolumeObject> object = storage_.volume_object(*id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::VolumeObjectConst> ModelTree::current_volume_const() const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::optional<ObjectId> id = current_item();
        if (!id)
        {
                return std::nullopt;
        }
        std::optional<storage::VolumeObjectConst> object = storage_.volume_object_const(*id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::VolumeObjectConst> ModelTree::volume_const_if_current(const ObjectId id) const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::optional<ObjectId> current_id = current_item();
        if (!current_id || id != current_id)
        {
                return std::nullopt;
        }
        std::optional<storage::VolumeObjectConst> object = storage_.volume_object_const(id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::VolumeObject> ModelTree::volume_if_current(const ObjectId id) const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::optional<ObjectId> current_id = current_item();
        if (!current_id || id != current_id)
        {
                return std::nullopt;
        }
        std::optional<storage::VolumeObject> object = storage_.volume_object(id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::vector<storage::MeshObjectConst> ModelTree::const_mesh_objects() const
{
        return storage_.objects<storage::MeshObjectConst>();
}

std::vector<storage::VolumeObjectConst> ModelTree::const_volume_objects() const
{
        return storage_.objects<storage::VolumeObjectConst>();
}

template <std::size_t N>
void ModelTree::set_visible(mesh::MeshObject<N>* const object, const bool visible)
{
        mesh::Writing writing(object);
        writing.set_visible(visible);
}

template <std::size_t N>
void ModelTree::set_visible(volume::VolumeObject<N>* const object, const bool visible)
{
        volume::Writing writing(object);
        writing.set_visible(visible);
}

template <typename T>
void ModelTree::make_menu_for_object(QMenu* const menu, const std::shared_ptr<T>& object)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const auto item_iter = map_id_item_.find(object->id());
        if (item_iter == map_id_item_.cend())
        {
                return;
        }

        {
                QAction* const action = menu->addAction("Show only it");
                QObject::connect(
                        action, &QAction::triggered,
                        [&]()
                        {
                                show_only_this_object(object->id());
                        });
        }
        {
                const bool visible = item_iter->second.visible;
                QAction* const action = visible ? menu->addAction("Hide") : menu->addAction("Show");
                QObject::connect(
                        action, &QAction::triggered,
                        [&, visible]()
                        {
                                set_visible(object.get(), !visible);
                        });
        }

        menu->addSeparator();

        {
                QAction* const action = menu->addAction("Delete");
                QObject::connect(
                        action, &QAction::triggered,
                        [&]()
                        {
                                std::optional<bool> yes = dialog::message_question_default_no("Delete?");
                                if (yes && *yes)
                                {
                                        object->erase();
                                }
                        });
        }
        {
                QAction* const action = menu->addAction("Delete All");
                QObject::connect(
                        action, &QAction::triggered,
                        [&]()
                        {
                                std::optional<bool> yes = dialog::message_question_default_no("Delete All?");
                                if (yes && *yes)
                                {
                                        clear();
                                }
                        });
        }
}

void ModelTree::make_menu(const QPoint& pos)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        QTreeWidgetItem* const item = ui_.model_tree->itemAt(pos);
        if (item != ui_.model_tree->currentItem())
        {
                return;
        }

        std::optional<storage::VolumeObject> volume = ModelTree::current_volume();
        std::optional<storage::MeshObject> mesh = ModelTree::current_mesh();
        ASSERT(!volume || !mesh);
        if (!volume && !mesh)
        {
                return;
        }

        const std::unique_ptr<QMenu> menu = std::make_unique<QMenu>();

        if (volume)
        {
                std::visit(
                        [&]<std::size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object)
                        {
                                make_menu_for_object(menu.get(), object);
                        },
                        *volume);
        }
        else if (mesh)
        {
                std::visit(
                        [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object)
                        {
                                make_menu_for_object(menu.get(), object);
                        },
                        *mesh);
        }

        if (menu->actions().empty())
        {
                return;
        }

        menu->exec(ui_.model_tree->mapToGlobal(pos));
}
}
