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

#include "event.h"

#include "repository/points.h"

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
                std::shared_ptr<const mesh::MeshObject<N>> object;
                std::shared_ptr<const SpatialMeshModel<N, MeshFloat>> mesh;
                std::shared_ptr<const std::vector<Vector<N, float>>> manifold_constructor_points;
                std::shared_ptr<const ManifoldConstructor<N>> manifold_constructor;
        };

        mutable std::shared_mutex m_mutex;
        std::unordered_map<ObjectId, MeshData> m_map;

public:
        explicit Storage(const std::function<void(StorageEvent&&)> events) : m_events(events)
        {
        }

        //

        template <typename T>
        void set_object(T&& object)
        {
                std::shared_ptr<const mesh::MeshObject<N>> tmp;
                {
                        std::unique_lock lock(m_mutex);
                        MeshData& v = m_map.try_emplace(object->id()).first->second;
                        tmp = std::move(v.object);
                        v.object = std::forward<T>(object);
                }
                m_events(StorageEvent::LoadedObject(object->id(), N));
        }

        template <typename T>
        void set_mesh(ObjectId id, T&& mesh)
        {
                std::shared_ptr<const SpatialMeshModel<N, MeshFloat>> tmp;
                {
                        std::unique_lock lock(m_mutex);
                        MeshData& v = m_map.try_emplace(id).first->second;
                        tmp = std::move(v.mesh);
                        v.mesh = std::forward<T>(mesh);
                }
                m_events(StorageEvent::LoadedMesh(id, N));
        }

        void set_points(ObjectId id, const std::shared_ptr<const std::vector<Vector<N, float>>>& points)
        {
                std::shared_ptr<const std::vector<Vector<N, float>>> tmp;
                std::unique_lock lock(m_mutex);
                MeshData& v = m_map.try_emplace(id).first->second;
                tmp = std::move(v.manifold_constructor_points);
                v.manifold_constructor_points = points;
        }

        void set_constructor(ObjectId id, const std::shared_ptr<const ManifoldConstructor<N>>& constructor)
        {
                std::shared_ptr<const ManifoldConstructor<N>> tmp;
                std::unique_lock lock(m_mutex);
                MeshData& v = m_map.try_emplace(id).first->second;
                tmp = std::move(v.manifold_constructor);
                v.manifold_constructor = constructor;
        }

        //

        std::shared_ptr<const mesh::MeshObject<N>> object(ObjectId id) const
        {
                std::shared_lock lock(m_mutex);
                auto iter = m_map.find(id);
                return (iter != m_map.cend()) ? iter->second.object : nullptr;
        }

        std::shared_ptr<const SpatialMeshModel<N, MeshFloat>> mesh(ObjectId id) const
        {
                std::shared_lock lock(m_mutex);
                auto iter = m_map.find(id);
                return (iter != m_map.cend()) ? iter->second.mesh : nullptr;
        }

        std::shared_ptr<const std::vector<Vector<N, float>>> points(ObjectId id) const
        {
                std::shared_lock lock(m_mutex);
                auto iter = m_map.find(id);
                return (iter != m_map.cend()) ? iter->second.manifold_constructor_points : nullptr;
        }

        std::shared_ptr<const ManifoldConstructor<N>> constructor(ObjectId id) const
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
