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

#include "options.h"
#include "storage.h"

#include <src/com/sequence.h>

#include <set>
#include <string>
#include <tuple>
#include <variant>

namespace storage
{
class MultiStorage final
{
        static constexpr int COUNT = MAXIMUM_DIMENSION - MINIMUM_DIMENSION + 1;
        static_assert(COUNT > 0);

        // std::tuple<T<MIN>, ..., T<MAX>>.
        using Tuple = Sequence<std::tuple, MINIMUM_DIMENSION, MAXIMUM_DIMENSION, Storage>;

        Tuple m_data;

        template <size_t N1, size_t N2>
        bool set_storage_object(
                const std::shared_ptr<mesh::MeshObject<N1>>& mesh_object,
                [[maybe_unused]] Storage<N2>* storage)
        {
                if constexpr (N1 == N2)
                {
                        storage->set_mesh_object(mesh_object);
                        return true;
                }
                return false;
        }

        template <size_t N1, size_t N2>
        bool set_storage_object(
                const std::shared_ptr<volume::VolumeObject<N1>>& volume_object,
                [[maybe_unused]] Storage<N2>* storage)
        {
                if constexpr (N1 == N2)
                {
                        storage->set_volume_object(volume_object);
                        return true;
                }
                return false;
        }

        template <size_t N>
        using MeshObjectPtr = std::shared_ptr<mesh::MeshObject<N>>;

        template <size_t N>
        using MeshObjectConstPtr = std::shared_ptr<const mesh::MeshObject<N>>;

        template <size_t N>
        using VolumeObjectPtr = std::shared_ptr<volume::VolumeObject<N>>;

        template <size_t N>
        using VolumeObjectConstPtr = std::shared_ptr<const volume::VolumeObject<N>>;

public:
        using MeshObject = Sequence<std::variant, MINIMUM_DIMENSION, MAXIMUM_DIMENSION, MeshObjectPtr>;
        using MeshObjectConst = Sequence<std::variant, MINIMUM_DIMENSION, MAXIMUM_DIMENSION, MeshObjectConstPtr>;

        using VolumeObject = Sequence<std::variant, MINIMUM_DIMENSION, MAXIMUM_DIMENSION, VolumeObjectPtr>;
        using VolumeObjectConst = Sequence<std::variant, MINIMUM_DIMENSION, MAXIMUM_DIMENSION, VolumeObjectConstPtr>;

        //

        static std::set<unsigned> supported_dimensions()
        {
                std::set<unsigned> v;
                std::apply(
                        [&]<size_t... N>(const Dimension<N>&...) { (v.insert(N), ...); }, DIMENSIONS);
                return v;
        }

        //

        void delete_object(ObjectId id)
        {
                std::apply(
                        [&]<size_t... N>(Storage<N> & ... storage) { (storage.delete_object(id), ...); }, m_data);
        }

        void clear()
        {
                std::apply([](auto&... v) { (v.clear(), ...); }, m_data);
        }

        void set_mesh_object(const MeshObject& mesh_object)
        {
                std::visit(
                        [&]<size_t N1>(const std::shared_ptr<mesh::MeshObject<N1>>& mesh) {
                                std::apply(
                                        [&]<size_t... N2>(Storage<N2> & ... storage) {
                                                (set_storage_object(mesh, &storage) || ...);
                                        },
                                        m_data);
                        },
                        mesh_object);
        }

        void set_volume_object(const VolumeObject& volume_object)
        {
                std::visit(
                        [&]<size_t N1>(const std::shared_ptr<volume::VolumeObject<N1>>& volume) {
                                std::apply(
                                        [&]<size_t... N2>(Storage<N2> & ... storage) {
                                                (set_storage_object(volume, &storage) || ...);
                                        },
                                        m_data);
                        },
                        volume_object);
        }

        std::optional<MeshObject> mesh_object(ObjectId id) const
        {
                std::optional<MeshObject> opt;

                std::apply(
                        [&]<size_t... N>(const Storage<N>&... storage) {
                                ([&]() {
                                        auto ptr = storage.mesh_object(id);
                                        if (ptr)
                                        {
                                                opt = std::move(ptr);
                                                return true;
                                        }
                                        return false;
                                }()
                                 || ...);
                        },
                        m_data);

                return opt;
        }

        std::optional<MeshObjectConst> mesh_object_const(ObjectId id) const
        {
                std::optional<MeshObjectConst> opt;

                std::apply(
                        [&]<size_t... N>(const Storage<N>&... storage) {
                                ([&]() {
                                        auto ptr = storage.mesh_object(id);
                                        if (ptr)
                                        {
                                                opt = std::move(ptr);
                                                return true;
                                        }
                                        return false;
                                }()
                                 || ...);
                        },
                        m_data);

                return opt;
        }

        std::optional<VolumeObject> volume_object(ObjectId id) const
        {
                std::optional<VolumeObject> opt;

                std::apply(
                        [&]<size_t... N>(const Storage<N>&... storage) {
                                ([&]() {
                                        auto ptr = storage.volume_object(id);
                                        if (ptr)
                                        {
                                                opt = std::move(ptr);
                                                return true;
                                        }
                                        return false;
                                }()
                                 || ...);
                        },
                        m_data);

                return opt;
        }

        std::optional<VolumeObjectConst> volume_object_const(ObjectId id) const
        {
                std::optional<VolumeObjectConst> opt;

                std::apply(
                        [&]<size_t... N>(const Storage<N>&... storage) {
                                ([&]() {
                                        auto ptr = storage.volume_object(id);
                                        if (ptr)
                                        {
                                                opt = std::move(ptr);
                                                return true;
                                        }
                                        return false;
                                }()
                                 || ...);
                        },
                        m_data);

                return opt;
        }
};
}
