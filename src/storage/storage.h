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

#include "events.h"
#include "mesh_object.h"
#include "options.h"
#include "pointers.h"

#include <src/com/sequence.h>
#include <src/geometry/cocone/reconstruction.h>
#include <src/geometry/objects/points.h>
#include <src/painter/shapes/mesh.h>

#include <memory>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

template <size_t N, typename MeshFloat>
class ObjectStorage
{
        static_assert(N >= 3);

        const StorageEvents& m_events;

        Pointers<ObjectId, const MeshObject<N>> m_objects;
        Pointers<ObjectId, const SpatialMeshModel<N, MeshFloat>> m_meshes;
        Pointers<ObjectId, const std::vector<Vector<N, float>>> m_manifold_constructors_points;
        Pointers<ObjectId, const ManifoldConstructor<N>> m_manifold_constructors;

public:
        explicit ObjectStorage(const StorageEvents& events) : m_events(events)
        {
        }

        //

        template <typename SetType>
        void set_object(SetType&& object)
        {
                m_objects.set(object->id(), std::forward<SetType>(object));
                m_events.loaded_object(object->id(), N);
        }

        template <typename SetType>
        void set_mesh(ObjectId id, SetType&& mesh)
        {
                m_meshes.set(id, std::forward<SetType>(mesh));
                m_events.loaded_mesh(id, N);
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

        std::shared_ptr<const MeshObject<N>> object(ObjectId id) const
        {
                return m_objects.get(id);
        }

        std::shared_ptr<const SpatialMeshModel<N, MeshFloat>> mesh(ObjectId id) const
        {
                return m_meshes.get(id);
        }

        std::shared_ptr<const std::vector<Vector<N, float>>> points(ObjectId id) const
        {
                return m_manifold_constructors_points.get(id);
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

                m_events.deleted_all(N);
        }
};

//

template <size_t DIMENSION>
struct ObjectStorageWithRepository final
{
        static constexpr size_t N = DIMENSION;

        const std::unique_ptr<const ObjectRepository<N>> repository;
        ObjectStorage<N, StorageMeshFloatingPoint> storage;

        explicit ObjectStorageWithRepository(const StorageEvents& storage_events)
                : repository(create_object_repository<N>()), storage(storage_events)
        {
        }
};

class ObjectMultiStorage
{
        // std::tuple<T<MIN>, ..., T<MAX>>.
        using Tuple =
                SequenceType1<std::tuple, STORAGE_MIN_DIMENSIONS, STORAGE_MAX_DIMENSIONS, ObjectStorageWithRepository>;

        Tuple m_data;

        template <size_t... I>
        ObjectMultiStorage(const StorageEvents& events, std::integer_sequence<size_t, I...>&&)
                : m_data((static_cast<void>(I), events)...)
        {
        }

public:
        using Data = Tuple;

        ObjectMultiStorage(const StorageEvents& events)
                : ObjectMultiStorage(
                          events,
                          std::make_integer_sequence<size_t, STORAGE_MAX_DIMENSIONS - STORAGE_MIN_DIMENSIONS + 1>())
        {
        }

        Data& data()
        {
                return m_data;
        }
};

using MeshVariant = SequenceType2ConstType2<
        std::variant,
        STORAGE_MIN_DIMENSIONS,
        STORAGE_MAX_DIMENSIONS,
        std::shared_ptr,
        SpatialMeshModel,
        StorageMeshFloatingPoint>;

using ObjectVariant = SequenceType2ConstType2<
        std::variant,
        STORAGE_MIN_DIMENSIONS,
        STORAGE_MAX_DIMENSIONS,
        std::shared_ptr,
        MeshObject>;
