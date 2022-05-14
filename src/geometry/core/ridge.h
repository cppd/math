/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/hash.h>
#include <src/com/print.h>
#include <src/com/sort.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <array>
#include <list>

namespace ns::geometry
{
template <std::size_t N>
class Ridge final
{
        static_assert(N > 1);
        std::array<int, N - 1> vertices_;

public:
        explicit Ridge(std::array<int, N - 1>&& vertices) : vertices_(vertices)
        {
                ASSERT(std::is_sorted(vertices.cbegin(), vertices.cend()));
        }

        bool operator==(const Ridge& a) const
        {
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        if (vertices_[i] != a.vertices_[i])
                        {
                                return false;
                        }
                }
                return true;
        }

        bool operator<(const Ridge& a) const
        {
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        if (vertices_[i] < a.vertices_[i])
                        {
                                return true;
                        }
                        if (vertices_[i] > a.vertices_[i])
                        {
                                return false;
                        }
                }
                return false;
        }

        const std::array<int, N - 1>& vertices() const
        {
                return vertices_;
        }

        std::size_t hash() const
        {
                return compute_hash(vertices_);
        }
};

template <typename Facet>
class RidgeDataElement final
{
        const Facet* facet_ = nullptr;
        int external_vertex_index_;

public:
        RidgeDataElement()
        {
        }

        RidgeDataElement(const Facet* const facet, const int external_vertex_index)
                : facet_(facet),
                  external_vertex_index_(external_vertex_index)
        {
        }

        int point() const
        {
                return facet_->vertices()[external_vertex_index_];
        }

        int vertex_index() const
        {
                return external_vertex_index_;
        }

        const Facet* facet() const
        {
                return facet_;
        }
};

template <typename Facet>
class RidgeData2 final
{
        static constexpr std::size_t SIZE = 2;

        std::array<RidgeDataElement<Facet>, SIZE> data_;

public:
        RidgeData2(const Facet* const facet, const int external_point_index) : data_{{{facet, external_point_index}}}
        {
        }

        void add(const Facet* const facet, const int external_point_index)
        {
                constexpr int INDEX = 1;

                if (!data_[INDEX].facet())
                {
                        data_[INDEX] = {facet, external_point_index};
                        return;
                }

                error("Add to ridge: too many facets exist in the link: facet " + to_string(facet->vertices())
                      + ", index " + to_string(external_point_index) + ", not ridge point "
                      + to_string(facet->vertices()[external_point_index]));
        }

        const RidgeDataElement<Facet>& operator[](const std::size_t index) const
        {
                ASSERT(index < SIZE);
                return data_[index];
        }
};

template <typename Facet>
class RidgeDataN final
{
        std::list<RidgeDataElement<Facet>> data_;

public:
        RidgeDataN(const Facet* const facet, const int external_point_index)
        {
                data_.emplace_back(facet, external_point_index);
        }

        void add(const Facet* const facet, const int external_point_index)
        {
                data_.emplace_back(facet, external_point_index);
        }

        void remove(const Facet* const facet)
        {
                for (auto iter = data_.cbegin(); iter != data_.cend(); ++iter)
                {
                        if (iter->facet() == facet)
                        {
                                data_.erase(iter);
                                return;
                        }
                }
                error("Remove ridge facet: facet not found in the link. Facet " + to_string(facet->vertices()));
        }

        auto cbegin() const
        {
                return data_.cbegin();
        }

        auto cend() const
        {
                return data_.cend();
        }

        bool empty() const
        {
                return data_.empty();
        }

        int size() const
        {
                return data_.size();
        }
};

template <std::size_t N, typename Facet, template <typename...> typename Map, typename MapData>
void add_to_ridges(const Facet& facet, Map<Ridge<N>, MapData>* const map)
{
        for (unsigned i = 0; i < N; ++i)
        {
                Ridge<N> ridge(sort(del_elem(facet.vertices(), i)));

                const auto iter = map->find(ridge);
                if (iter == map->end())
                {
                        map->emplace(std::move(ridge), MapData(&facet, i));
                }
                else
                {
                        iter->second.add(&facet, i);
                }
        }
}

template <std::size_t N, typename Facet, template <typename...> typename Map, typename MapData>
void remove_from_ridges(const Facet& facet, Map<Ridge<N>, MapData>* const map)
{
        for (unsigned i = 0; i < N; ++i)
        {
                const auto iter = map->find(Ridge<N>(sort(del_elem(facet.vertices(), i))));
                ASSERT(iter != map->end());

                iter->second.remove(&facet);
                if (iter->second.empty())
                {
                        map->erase(iter);
                }
        }
}

template <std::size_t N, typename Facet, template <typename...> typename Set>
void add_to_ridges(const Facet& facet, const int exclude_point, Set<Ridge<N>>* const ridges)
{
        const std::array<int, N>& vertices = facet.vertices();

        for (unsigned i = 0; i < N; ++i)
        {
                if (vertices[i] != exclude_point)
                {
                        ridges->emplace(sort(del_elem(vertices, i)));
                }
        }
}
}

namespace std
{
template <std::size_t N>
struct hash<::ns::geometry::Ridge<N>> final
{
        std::size_t operator()(const ::ns::geometry::Ridge<N>& v) const
        {
                return v.hash();
        }
};
}
