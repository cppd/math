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

#include "storage_event.h"

#include <src/geometry/reconstruction/cocone.h>
#include <src/model/mesh_object.h>
#include <src/painter/shapes/mesh.h>

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

template <size_t N, typename MeshFloat>
class Storage
{
        static_assert(N >= 3);

        const std::function<void(StorageEvent&&)> m_events;

        struct MeshData
        {
                std::shared_ptr<const mesh::MeshObject<N>> mesh_object;
                std::shared_ptr<const painter::MeshObject<N, MeshFloat>> painter_mesh_object;
                std::shared_ptr<const geometry::ManifoldConstructor<N>> manifold_constructor;
        };

        mutable std::shared_mutex m_mutex;
        std::unordered_map<ObjectId, MeshData> m_map;

public:
        static constexpr size_t DIMENSION = N;

        explicit Storage(const std::function<void(StorageEvent&&)> events) : m_events(events)
        {
        }

        //

        template <typename T>
        void set_mesh_object(T&& object)
        {
                std::shared_ptr<const mesh::MeshObject<N>> tmp;
                {
                        std::unique_lock lock(m_mutex);
                        MeshData& v = m_map.try_emplace(object->id()).first->second;
                        tmp = std::move(v.mesh_object);
                        v.mesh_object = std::forward<T>(object);
                }
                m_events(StorageEvent::LoadedMeshObject(object->id(), N));
        }

        template <typename T>
        void set_painter_mesh_object(ObjectId id, T&& mesh)
        {
                std::shared_ptr<const painter::MeshObject<N, MeshFloat>> tmp;
                {
                        std::unique_lock lock(m_mutex);
                        MeshData& v = m_map.try_emplace(id).first->second;
                        tmp = std::move(v.painter_mesh_object);
                        v.painter_mesh_object = std::forward<T>(mesh);
                }
                m_events(StorageEvent::LoadedPainterMeshObject(id, N));
        }

        void set_manifold_constructor(
                ObjectId id,
                const std::shared_ptr<const geometry::ManifoldConstructor<N>>& manifold_constructor)
        {
                std::shared_ptr<const geometry::ManifoldConstructor<N>> tmp;
                std::unique_lock lock(m_mutex);
                MeshData& v = m_map.try_emplace(id).first->second;
                tmp = std::move(v.manifold_constructor);
                v.manifold_constructor = manifold_constructor;
        }

        //

        std::shared_ptr<const mesh::MeshObject<N>> mesh_object(ObjectId id) const
        {
                std::shared_lock lock(m_mutex);
                auto iter = m_map.find(id);
                return (iter != m_map.cend()) ? iter->second.mesh_object : nullptr;
        }

        std::shared_ptr<const painter::MeshObject<N, MeshFloat>> painter_mesh_object(ObjectId id) const
        {
                std::shared_lock lock(m_mutex);
                auto iter = m_map.find(id);
                return (iter != m_map.cend()) ? iter->second.painter_mesh_object : nullptr;
        }

        std::shared_ptr<const geometry::ManifoldConstructor<N>> manifold_constructor(ObjectId id) const
        {
                std::shared_lock lock(m_mutex);
                auto iter = m_map.find(id);
                return (iter != m_map.cend()) ? iter->second.manifold_constructor : nullptr;
        }

        //

        void clear()
        {
                std::unordered_map<ObjectId, MeshData> tmp;
                {
                        std::unique_lock lock(m_mutex);
                        tmp = std::move(m_map);
                        m_map.clear();
                }
                m_events(StorageEvent::DeletedAll(N));
        }
};
