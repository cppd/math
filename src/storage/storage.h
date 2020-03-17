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

#include "mesh_object.h"
#include "pointers.h"
#include "storage_events.h"

#include <src/geometry/cocone/reconstruction.h>
#include <src/geometry/objects/points.h>
#include <src/painter/shapes/mesh.h>

#include <memory>
#include <string>
#include <vector>

template <size_t N, typename MeshFloat>
class ObjectStorage
{
        static_assert(N >= 3);

        const ObjectStorageEvents& m_event_emitter;

        const std::unique_ptr<ObjectRepository<N>> m_object_repository;

        Pointers<ObjectId, const MeshObject<N>> m_objects;
        Pointers<ObjectId, const SpatialMeshModel<N, MeshFloat>> m_meshes;
        Pointers<ObjectId, const std::vector<Vector<N, float>>> m_manifold_constructors_points;
        Pointers<ObjectId, const ManifoldConstructor<N>> m_manifold_constructors;

public:
        ObjectStorage(const ObjectStorageEvents& events)
                : m_event_emitter(events), m_object_repository(create_object_repository<N>())
        {
        }

        //

        std::vector<std::string> repository_point_object_names() const
        {
                return m_object_repository->point_object_names();
        }

        std::vector<Vector<N, float>> repository_point_object(const std::string& object_name, unsigned point_count)
                const
        {
                return m_object_repository->point_object(object_name, point_count);
        }

        //

        template <typename SetType>
        void set_object(SetType&& object)
        {
                m_objects.set(object->id(), std::forward<SetType>(object));
                m_event_emitter.object_loaded(object->id(), N);
        }

        template <typename SetType>
        void set_mesh(ObjectId id, SetType&& mesh)
        {
                m_meshes.set(id, std::forward<SetType>(mesh));
                m_event_emitter.mesh_loaded(id, N);
        }

        void set_points(ObjectId id, const std::shared_ptr<const std::vector<Vector<N, float>>>& points)
        {
                m_manifold_constructors_points.set(id, points);
        }

        void set_constructor(ObjectId id, const std::shared_ptr<const ManifoldConstructor<N>>& constructor)
        {
                m_manifold_constructors.set(id, constructor);
        }

        //

        bool object_exists(ObjectId id) const
        {
                return m_objects.get(id) != nullptr;
        }
        std::shared_ptr<const MeshObject<N>> object(ObjectId id) const
        {
                return m_objects.get(id);
        }

        bool mesh_exists(ObjectId id) const
        {
                return m_meshes.get(id) != nullptr;
        }
        std::shared_ptr<const SpatialMeshModel<N, MeshFloat>> mesh(ObjectId id) const
        {
                return m_meshes.get(id);
        }

        bool points_exist(ObjectId id) const
        {
                return m_manifold_constructors_points.get(id) != nullptr;
        }
        std::shared_ptr<const std::vector<Vector<N, float>>> points(ObjectId id) const
        {
                return m_manifold_constructors_points.get(id);
        }

        bool constructor_exists(ObjectId id) const
        {
                return m_manifold_constructors.get(id) != nullptr;
        }
        std::shared_ptr<const ManifoldConstructor<N>> constructor(ObjectId id) const
        {
                return m_manifold_constructors.get(id);
        }

        //

        void clear()
        {
                m_manifold_constructors_points.clear();
                m_manifold_constructors.clear();
                m_objects.clear();
                m_meshes.clear();

                m_event_emitter.object_deleted_all(N);
        }
};
