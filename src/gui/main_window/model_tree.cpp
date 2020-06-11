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

#include <QMenu>
#include <memory>

namespace gui
{
ModelTree::ModelTree(QTreeWidget* tree, const std::function<void()>& item_changed)
        : m_thread_id(std::this_thread::get_id()), m_tree(tree)
{
        ASSERT(m_tree);

        m_connections.emplace_back(QObject::connect(
                m_tree, &QTreeWidget::currentItemChanged, [=](QTreeWidgetItem*, QTreeWidgetItem*) { item_changed(); }));

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

void ModelTree::insert(const storage::MeshObject& object, const std::optional<ObjectId>& /*parent_object_id*/)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<mesh::MeshObject<N>>& mesh) {
                        insert_into_tree(mesh->id(), N, mesh->name());
                        m_storage.set_mesh_object(mesh);
                },
                object);
}

void ModelTree::insert(const storage::VolumeObject& object, const std::optional<ObjectId>& /*parent_object_id*/)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::visit(
                [&]<size_t N>(const std::shared_ptr<volume::VolumeObject<N>>& volume) {
                        insert_into_tree(volume->id(), N, volume->name());
                        m_storage.set_volume_object(volume);
                },
                object);
}

void ModelTree::insert_into_tree(ObjectId id, unsigned dimension, const std::string& name)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        auto iter = m_map_id_item.find(id);
        if (iter != m_map_id_item.cend())
        {
                return;
        }
        std::unique_ptr<QTreeWidgetItem> item = std::make_unique<QTreeWidgetItem>();
        QString s = QString("(%1D) %2").arg(dimension).arg(QString::fromStdString(name));
        item->setText(0, s);
        item->setToolTip(0, s);
        QTreeWidgetItem* ptr = item.get();
        m_tree->addTopLevelItem(item.release());
        m_map_item_id[ptr] = id;
        m_map_id_item[id] = ptr;
}

void ModelTree::erase(ObjectId id)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        m_storage.delete_object(id);

        auto iter = m_map_id_item.find(id);
        if (iter == m_map_id_item.cend())
        {
                return;
        }
        QTreeWidgetItem* item = iter->second;
        m_map_id_item.erase(id);
        m_map_item_id.erase(item);
        int index = m_tree->indexOfTopLevelItem(item);
        ASSERT(index >= 0);
        if (index >= 0)
        {
                delete m_tree->takeTopLevelItem(index);
        }
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

void ModelTree::make_menu(const QPoint& pos)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        QTreeWidgetItem* item = m_tree->itemAt(pos);

        auto iter = m_map_item_id.find(item);
        if (iter == m_map_item_id.cend())
        {
                return;
        }

        ObjectId id = iter->second;

        std::unique_ptr<QMenu> menu = std::make_unique<QMenu>();
        QAction* action = menu->addAction("Delete");
        QObject::connect(action, &QAction::triggered, [id, this]() { erase(id); });
        menu->exec(m_tree->mapToGlobal(pos));
}
}
