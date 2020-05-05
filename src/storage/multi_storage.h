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
        using Tuple = SequenceType1<std::tuple, MINIMUM_DIMENSION, MAXIMUM_DIMENSION, Storage>;

        Tuple m_data;

public:
        using Data = Tuple;

        using MeshObject =
                SequenceType2<std::variant, MINIMUM_DIMENSION, MAXIMUM_DIMENSION, std::shared_ptr, mesh::MeshObject>;

        using MeshObjectConst = SequenceType2Const2<
                std::variant,
                MINIMUM_DIMENSION,
                MAXIMUM_DIMENSION,
                std::shared_ptr,
                mesh::MeshObject>;

        //

        using VolumeObject =
                SequenceType2<std::variant, MINIMUM_DIMENSION, MAXIMUM_DIMENSION, std::shared_ptr, volume::VolumeObject>;

        using VolumeObjectConst = SequenceType2Const2<
                std::variant,
                MINIMUM_DIMENSION,
                MAXIMUM_DIMENSION,
                std::shared_ptr,
                volume::VolumeObject>;

        //

        static std::set<unsigned> supported_dimensions()
        {
                std::set<unsigned> v;
                for (int d = MINIMUM_DIMENSION; d <= MAXIMUM_DIMENSION; ++d)
                {
                        v.insert(d);
                }
                return v;
        }

        //

        Data& data()
        {
                return m_data;
        }

        const Data& data() const
        {
                return m_data;
        }

        void clear()
        {
                std::apply([](auto&... v) { (v.clear(), ...); }, m_data);
        }

        std::optional<MeshObject> mesh_object(ObjectId id) const
        {
                std::optional<MeshObject> opt;

                std::apply(
                        [&](const auto&... v) {
                                ([&]() {
                                        auto ptr = v.mesh_object(id);
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
                        [&](const auto&... v) {
                                ([&]() {
                                        auto ptr = v.mesh_object(id);
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
                        [&](const auto&... v) {
                                ([&]() {
                                        auto ptr = v.volume_object(id);
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
                        [&](const auto&... v) {
                                ([&]() {
                                        auto ptr = v.volume_object(id);
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
