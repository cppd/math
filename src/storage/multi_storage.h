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

class MultiStorage final
{
        static_assert(STORAGE_MIN_DIMENSION >= 3 && STORAGE_MIN_DIMENSION <= STORAGE_MAX_DIMENSION);
        static constexpr int COUNT = STORAGE_MAX_DIMENSION - STORAGE_MIN_DIMENSION + 1;

        using PainterFloatingPoint = double;

        static_assert(COUNT > 0);
        static_assert(std::is_floating_point_v<PainterFloatingPoint>);

        // std::tuple<T<MIN>, ..., T<MAX>>.
        using Tuple =
                SequenceType1<std::tuple, STORAGE_MIN_DIMENSION, STORAGE_MAX_DIMENSION, Storage, PainterFloatingPoint>;

        Tuple m_data;

        template <size_t... I>
        MultiStorage(const std::function<void(StorageEvent&&)>& events, std::integer_sequence<size_t, I...>&&)
                : m_data((static_cast<void>(I), events)...)
        {
        }

public:
        using Data = Tuple;

        using MeshObject = SequenceType2ConstType2<
                std::variant,
                STORAGE_MIN_DIMENSION,
                STORAGE_MAX_DIMENSION,
                std::shared_ptr,
                mesh::MeshObject>;

        using PainterMeshObject = SequenceType2ConstType2<
                std::variant,
                STORAGE_MIN_DIMENSION,
                STORAGE_MAX_DIMENSION,
                std::shared_ptr,
                painter::MeshObject,
                PainterFloatingPoint>;

        using VolumeObject = SequenceType2ConstType2<
                std::variant,
                STORAGE_MIN_DIMENSION,
                STORAGE_MAX_DIMENSION,
                std::shared_ptr,
                volume::VolumeObject>;

        static std::set<unsigned> supported_dimensions()
        {
                std::set<unsigned> v;
                for (int d = STORAGE_MIN_DIMENSION; d <= STORAGE_MAX_DIMENSION; ++d)
                {
                        v.insert(d);
                }
                return v;
        }

        explicit MultiStorage(const std::function<void(StorageEvent&&)>& events)
                : MultiStorage(events, std::make_integer_sequence<size_t, COUNT>())
        {
        }

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
                                }() ||
                                 ...);
                        },
                        m_data);

                return opt;
        }

        std::optional<PainterMeshObject> painter_mesh_object(ObjectId id) const
        {
                std::optional<PainterMeshObject> opt;

                std::apply(
                        [&](const auto&... v) {
                                ([&]() {
                                        auto ptr = v.painter_mesh_object(id);
                                        if (ptr)
                                        {
                                                opt = std::move(ptr);
                                                return true;
                                        }
                                        return false;
                                }() ||
                                 ...);
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
                                }() ||
                                 ...);
                        },
                        m_data);

                return opt;
        }
};
