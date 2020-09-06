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

#include "../dialogs/message.h"

#include <src/com/error.h>

#include <QMenu>
#include <memory>

namespace gui
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

ModelTree::ModelTree(QTreeWidget* tree, const std::function<void()>& on_item_changed)
        : m_thread_id(std::this_thread::get_id()), m_tree(tree), m_on_item_changed(on_item_changed)
{
        ASSERT(m_tree);

        m_connections.emplace_back(
                QObject::connect(m_tree, &QTreeWidget::currentItemChanged, [this](QTreeWidgetItem*, QTreeWidgetItem*) {
                        m_on_item_changed();
                }));

        m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
        m_connections.emplace_back(QObject::connect(
                m_tree, &QTreeWidget::customContextMenuRequested, [this](const QPoint& p) { make_menu(p); }));
}

ModelTree::~ModelTree()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        m_connections.clear();

        clear();
}

void ModelTree::clear()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        m_storage.clear();
        m_map_item_id.clear();
        m_map_id_item.clear();
        m_tree->clear();
}

void ModelTree::insert_into_tree_and_storage(
        const storage::MeshObject& object,
        const std::optional<ObjectId>& parent_object_id)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& mesh) {
                        insert_into_tree(mesh->id(), N, mesh->name(), mesh->visible(), parent_object_id);
                        m_storage.set_mesh_object(mesh);
                },
                object);
}

void ModelTree::insert_into_tree_and_storage(
        const storage::VolumeObject& object,
        const std::optional<ObjectId>& parent_object_id)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume) {
                        insert_into_tree(volume->id(), N, volume->name(), volume->visible(), parent_object_id);
                        m_storage.set_volume_object(volume);
                },
                object);
}

void ModelTree::erase_from_tree_and_storage(ObjectId id)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        m_storage.delete_object(id);
        erase_from_tree(id);
}

void ModelTree::update(ObjectId id)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<ObjectId> current_id = current_item();
        if (!current_id)
        {
                return;
        }
        if (id != *current_id)
        {
                return;
        }
        m_on_item_changed();
}

void ModelTree::insert_into_tree(
        ObjectId id,
        unsigned dimension,
        const std::string& name,
        bool visible,
        const std::optional<ObjectId>& parent_object_id)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        auto iter = m_map_id_item.find(id);
        if (iter != m_map_id_item.cend())
        {
                return;
        }

        QTreeWidgetItem* parent_item = nullptr;
        if (parent_object_id)
        {
                auto parent_iter = m_map_id_item.find(*parent_object_id);
                if (parent_iter != m_map_id_item.cend())
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
                item = new QTreeWidgetItem(m_tree);
        }

        QString s = QString("(%1D) %2").arg(dimension).arg(QString::fromStdString(name));
        item->setText(0, s);
        item->setToolTip(0, s);

        set_item_color(item, visible);

        m_map_item_id[item] = id;
        m_map_id_item[id] = item;
}

void ModelTree::erase_from_tree(ObjectId id)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        auto iter = m_map_id_item.find(id);
        if (iter == m_map_id_item.cend())
        {
                return;
        }
        QTreeWidgetItem* item = iter->second;

        m_map_id_item.erase(id);
        m_map_item_id.erase(item);

        if (item->childCount() > 0)
        {
                QFont font = item->font(0);
                font.setStrikeOut(true);
                item->setFont(0, font);
                set_item_color(item, false);
                m_on_item_changed();
        }
        else
        {
                QTreeWidgetItem* parent = item->parent();
                delete item;
                while (parent && parent->childCount() == 0)
                {
                        item = parent;
                        parent = item->parent();
                        ASSERT(m_map_item_id.count(item) == 0);
                        delete item;
                }
        }
}

void ModelTree::set_visible_in_tree(ObjectId id, bool visible)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        auto iter = m_map_id_item.find(id);
        if (iter == m_map_id_item.cend())
        {
                return;
        }
        set_item_color(iter->second, visible);
}

void ModelTree::show_object(ObjectId id, bool show)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<storage::MeshObject> m = m_storage.mesh_object(id);
        std::optional<storage::VolumeObject> v = m_storage.volume_object(id);

        if (m.has_value() && v.has_value())
        {
                error_fatal("Mesh and volume with the same id");
        }

        if (v)
        {
                std::visit(
                        [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume_object) {
                                volume_object->set_visible(show);
                        },
                        *v);
        }
        else if (m)
        {
                std::visit(
                        [&]<size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& mesh_object) {
                                mesh_object->set_visible(show);
                        },
                        *m);
        }
}

void ModelTree::show_only_this_object(ObjectId id)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        for (const auto& v : m_map_id_item)
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
        ASSERT(std::this_thread::get_id() == m_thread_id);

        auto iter = m_map_item_id.find(m_tree->currentItem());
        if (iter != m_map_item_id.cend())
        {
                return iter->second;
        }
        return std::nullopt;
}

//void ModelTree::set_current(ObjectId id)
//{
//        auto iter = m_map_id_item.find(id);
//        if (iter == m_map_id_item.cend())
//        {
//                return;
//        }
//        m_tree->setCurrentItem(iter->second);
//}

std::optional<storage::MeshObject> ModelTree::current_mesh() const
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<ObjectId> id = current_item();
        if (!id)
        {
                return std::nullopt;
        }
        std::optional<storage::MeshObject> object = m_storage.mesh_object(*id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::MeshObjectConst> ModelTree::current_mesh_const() const
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<ObjectId> id = current_item();
        if (!id)
        {
                return std::nullopt;
        }
        std::optional<storage::MeshObjectConst> object = m_storage.mesh_object_const(*id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::MeshObjectConst> ModelTree::mesh_const_if_current(ObjectId id) const
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<ObjectId> current_id = current_item();
        if (!current_id || id != current_id)
        {
                return std::nullopt;
        }
        std::optional<storage::MeshObjectConst> object = m_storage.mesh_object_const(id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::MeshObject> ModelTree::mesh_if_current(ObjectId id) const
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<ObjectId> current_id = current_item();
        if (!current_id || id != current_id)
        {
                return std::nullopt;
        }
        std::optional<storage::MeshObject> object = m_storage.mesh_object(id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::VolumeObject> ModelTree::current_volume() const
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<ObjectId> id = current_item();
        if (!id)
        {
                return std::nullopt;
        }
        std::optional<storage::VolumeObject> object = m_storage.volume_object(*id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::VolumeObjectConst> ModelTree::current_volume_const() const
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<ObjectId> id = current_item();
        if (!id)
        {
                return std::nullopt;
        }
        std::optional<storage::VolumeObjectConst> object = m_storage.volume_object_const(*id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::VolumeObjectConst> ModelTree::volume_const_if_current(ObjectId id) const
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<ObjectId> current_id = current_item();
        if (!current_id || id != current_id)
        {
                return std::nullopt;
        }
        std::optional<storage::VolumeObjectConst> object = m_storage.volume_object_const(id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::optional<storage::VolumeObject> ModelTree::volume_if_current(ObjectId id) const
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::optional<ObjectId> current_id = current_item();
        if (!current_id || id != current_id)
        {
                return std::nullopt;
        }
        std::optional<storage::VolumeObject> object = m_storage.volume_object(id);
        if (!object)
        {
                return std::nullopt;
        }
        return object;
}

std::vector<storage::MeshObjectConst> ModelTree::const_mesh_objects() const
{
        return m_storage.objects<storage::MeshObjectConst>();
}

std::vector<storage::VolumeObjectConst> ModelTree::const_volume_objects() const
{
        return m_storage.objects<storage::VolumeObjectConst>();
}

template <typename T>
void ModelTree::make_menu_for_object(QMenu* menu, const std::shared_ptr<T>& object)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        {
                QAction* action = menu->addAction("Show only it");
                QObject::connect(action, &QAction::triggered, [&]() { show_only_this_object(object->id()); });
        }
        {
                bool visible = object->visible();
                QAction* action = visible ? menu->addAction("Hide") : menu->addAction("Show");
                QObject::connect(action, &QAction::triggered, [&, visible]() { object->set_visible(!visible); });
        }
        menu->addSeparator();
        {
                QAction* action = menu->addAction("Delete");
                QObject::connect(action, &QAction::triggered, [&]() {
                        bool yes;
                        if (dialog::message_question_default_no("Delete?", &yes) && yes)
                        {
                                object->erase();
                        }
                });
        }
        {
                QAction* action = menu->addAction("Delete All");
                QObject::connect(action, &QAction::triggered, [&]() {
                        bool yes;
                        if (dialog::message_question_default_no("Delete All?", &yes) && yes)
                        {
                                clear();
                        }
                });
        }
}

void ModelTree::make_menu(const QPoint& pos)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        QTreeWidgetItem* item = m_tree->itemAt(pos);
        if (item != m_tree->currentItem())
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
                        [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& object) {
                                make_menu_for_object(menu.get(), object);
                        },
                        *volume);
        }
        else if (mesh)
        {
                std::visit(
                        [&]<size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& object) {
                                make_menu_for_object(menu.get(), object);
                        },
                        *mesh);
        }

        if (menu->actions().empty())
        {
                return;
        }

        menu->exec(m_tree->mapToGlobal(pos));
}
}
