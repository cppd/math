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

#include "storage.h"

#include <src/com/sequence.h>

#include <set>
#include <string>
#include <tuple>
#include <variant>

template <size_t DIMENSION, typename MeshFloat>
struct StorageWithRepository final
{
        static constexpr size_t N = DIMENSION;

        const std::unique_ptr<const PointObjectRepository<N>> repository;
        Storage<N, MeshFloat> storage;

        explicit StorageWithRepository(const std::function<void(StorageEvent&&)>& storage_events)
                : repository(create_point_object_repository<N>()), storage(storage_events)
        {
        }
};

class MultiStorage final
{
        static constexpr int MIN = 3;
        static constexpr int MAX = 5;
        static constexpr int COUNT = MAX - MIN + 1;

        using MeshFloatingPoint = double;

        static_assert(MIN >= 3 && MIN <= MAX);
        static_assert(COUNT > 0);
        static_assert(std::is_floating_point_v<MeshFloatingPoint>);

        // std::tuple<T<MIN>, ..., T<MAX>>.
        using Tuple = SequenceType1<std::tuple, MIN, MAX, StorageWithRepository, MeshFloatingPoint>;

        Tuple m_data;

        template <size_t... I>
        MultiStorage(const std::function<void(StorageEvent&&)>& events, std::integer_sequence<size_t, I...>&&)
                : m_data((static_cast<void>(I), events)...)
        {
        }

public:
        using Data = Tuple;

        using MeshVariant =
                SequenceType2ConstType2<std::variant, MIN, MAX, std::shared_ptr, SpatialMeshModel, MeshFloatingPoint>;

        using ObjectVariant = SequenceType2ConstType2<std::variant, MIN, MAX, std::shared_ptr, mesh::MeshObject>;

        static std::set<unsigned> dimensions()
        {
                std::set<unsigned> v;
                for (int d = MIN; d <= MAX; ++d)
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
                std::apply([](auto&... v) { (v.storage.clear(), ...); }, m_data);
        }

        struct RepositoryObjects
        {
                int dimension;
                std::vector<std::string> names;
        };
        std::vector<RepositoryObjects> repository_point_object_names() const
        {
                std::vector<RepositoryObjects> names;

                std::apply(
                        [&](const auto&... v) {
                                (
                                        [&]() {
                                                names.resize(names.size() + 1);
                                                names.back().dimension = v.N;
                                                names.back().names = v.repository->point_object_names();
                                        }(),
                                        ...);
                        },
                        m_data);

                return names;
        }

        std::optional<ObjectVariant> object(ObjectId id) const
        {
                std::optional<ObjectVariant> opt;

                std::apply(
                        [&](const auto&... v) {
                                ([&]() {
                                        auto ptr = v.storage.object(id);
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

        std::optional<MeshVariant> mesh(ObjectId id) const
        {
                std::optional<MeshVariant> opt;

                std::apply(
                        [&](const auto&... v) {
                                ([&]() {
                                        auto ptr = v.storage.mesh(id);
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
