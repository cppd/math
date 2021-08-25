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

void set_item_color(QTreeWidgetItem* item, bool visible)
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

void ModelTree::insert(storage::MeshObject&& object, const std::optional<ObjectId>& parent_object_id)
{
        thread_queue_.push(
                [this, object = std::move(object), parent_object_id]()
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        std::visit(
                                [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& mesh)
                                {
                                        insert_into_tree(
                                                mesh->id(), N, mesh->name(), mesh->visible(), parent_object_id);
                                        storage_.set_mesh_object(mesh);
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
                                        insert_into_tree(
                                                volume->id(), N, volume->name(), volume->visible(), parent_object_id);
                                        storage_.set_volume_object(volume);
                                },
                                object);
                });
}

void ModelTree::erase(ObjectId id)
{
        thread_queue_.push(
                [this, id]()
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        storage_.delete_object(id);
                        erase_from_tree(id);
                });
}

void ModelTree::update(ObjectId id)
{
        thread_queue_.push(
                [this, id]()
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        std::optional<ObjectId> current_id = current_item();
                        if (!current_id)
                        {
                                return;
                        }
                        if (id != *current_id)
                        {
                                return;
                        }
                        Q_EMIT item_update();
                });
}

void ModelTree::show(ObjectId id, bool visible)
{
        thread_queue_.push(
                [this, id, visible]()
                {
                        ASSERT(std::this_thread::get_id() == thread_id_);

                        auto iter = map_id_item_.find(id);
                        if (iter == map_id_item_.cend())
                        {
                                return;
                        }
                        set_item_color(iter->second, visible);
                });
}

void ModelTree::insert_into_tree(
        ObjectId id,
        unsigned dimension,
        const std::string& name,
        bool visible,
        const std::optional<ObjectId>& parent_object_id)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        auto iter = map_id_item_.find(id);
        if (iter != map_id_item_.cend())
        {
                return;
        }

        QTreeWidgetItem* parent_item = nullptr;
        if (parent_object_id)
        {
                auto parent_iter = map_id_item_.find(*parent_object_id);
                if (parent_iter != map_id_item_.cend())
                {
                        parent_item = parent_iter->second;
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

        QString s = QString("(%1D) %2").arg(dimension).arg(QString::fromStdString(name));
        item->setText(0, s);
        item->setToolTip(0, s);

        set_item_color(item, visible);

        map_item_id_[item] = id;
        map_id_item_[id] = item;
}

void ModelTree::erase_from_tree(ObjectId id)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        auto iter = map_id_item_.find(id);
        if (iter == map_id_item_.cend())
        {
                return;
        }
        QTreeWidgetItem* item = iter->second;

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
                QTreeWidgetItem* parent = item->parent();
                delete item;
                while (parent && parent->childCount() == 0)
                {
                        item = parent;
                        parent = item->parent();
                        ASSERT(map_item_id_.count(item) == 0);
                        delete item;
                }
        }
}

void ModelTree::show_object(ObjectId id, bool show)
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
                                volume_object->set_visible(show);
                        },
                        *v);
        }
        else if (m)
        {
                std::visit(
                        [&]<std::size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& mesh_object)
                        {
                                mesh_object->set_visible(show);
                        },
                        *m);
        }
}

void ModelTree::show_only_this_object(ObjectId id)
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

//void ModelTree::set_current(ObjectId id)
//{
//        auto iter = map_id_item_.find(id);
//        if (iter == map_id_item_.cend())
//        {
//                return;
//        }
//        tree_->setCurrentItem(iter->second);
//}

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

std::optional<storage::MeshObjectConst> ModelTree::mesh_const_if_current(ObjectId id) const
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

std::optional<storage::MeshObject> ModelTree::mesh_if_current(ObjectId id) const
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

std::optional<storage::VolumeObjectConst> ModelTree::volume_const_if_current(ObjectId id) const
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

std::optional<storage::VolumeObject> ModelTree::volume_if_current(ObjectId id) const
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

template <typename T>
void ModelTree::make_menu_for_object(QMenu* menu, const std::shared_ptr<T>& object)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        {
                QAction* action = menu->addAction("Show only it");
                QObject::connect(
                        action, &QAction::triggered,
                        [&]()
                        {
                                show_only_this_object(object->id());
                        });
        }
        {
                bool visible = object->visible();
                QAction* action = visible ? menu->addAction("Hide") : menu->addAction("Show");
                QObject::connect(
                        action, &QAction::triggered,
                        [&, visible]()
                        {
                                object->set_visible(!visible);
                        });
        }
        menu->addSeparator();
        {
                QAction* action = menu->addAction("Delete");
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
                QAction* action = menu->addAction("Delete All");
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

        QTreeWidgetItem* item = ui_.model_tree->itemAt(pos);
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

        std::unique_ptr<QMenu> menu = std::make_unique<QMenu>();

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
