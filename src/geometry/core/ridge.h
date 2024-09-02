/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <vector>

namespace ns::geometry::core
{
template <std::size_t N>
class Ridge final
{
        static_assert(N > 1);

        std::array<int, N - 1> vertices_;

public:
        explicit Ridge(std::array<int, N - 1>&& vertices)
                : vertices_(vertices)
        {
                ASSERT(std::is_sorted(vertices.cbegin(), vertices.cend()));
        }

        [[nodiscard]] bool operator==(const Ridge& a) const
        {
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        if (vertices_[i] != a.vertices_[i])
                        {
                                return false;
                        }
                }
                return true;
        }

        [[nodiscard]] bool operator<(const Ridge& a) const
        {
                for (std::size_t i = 0; i < N - 1; ++i)
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

        [[nodiscard]] const std::array<int, N - 1>& vertices() const
        {
                return vertices_;
        }

        [[nodiscard]] std::size_t hash() const
        {
                return compute_hash(vertices_);
        }
};

template <typename Facet>
class RidgeFacet final
{
        const Facet* facet_ = nullptr;
        int external_vertex_index_;

public:
        RidgeFacet()
        {
        }

        RidgeFacet(const Facet* const facet, const int external_vertex_index)
                : facet_(facet),
                  external_vertex_index_(external_vertex_index)
        {
                ASSERT(facet);
        }

        [[nodiscard]] int point() const
        {
                return facet_->vertices()[external_vertex_index_];
        }

        [[nodiscard]] int vertex_index() const
        {
                return external_vertex_index_;
        }

        [[nodiscard]] const Facet* facet() const
        {
                return facet_;
        }
};

template <typename Facet>
class RidgeFacets2 final
{
        RidgeFacet<Facet> facet_0_;
        RidgeFacet<Facet> facet_1_;

public:
        RidgeFacets2(const Facet* const facet, const int external_point_index)
                : facet_0_(facet, external_point_index)
        {
        }

        void add(const Facet* const facet, const int external_point_index)
        {
                if (!facet_1_.facet())
                {
                        facet_1_ = {facet, external_point_index};
                        return;
                }

                error("Add to ridge: too many facets exist in the link: facet " + to_string(facet->vertices())
                      + ", index " + to_string(external_point_index) + ", not ridge point "
                      + to_string(facet->vertices()[external_point_index]));
        }

        [[nodiscard]] const RidgeFacet<Facet>& f0() const
        {
                return facet_0_;
        }

        [[nodiscard]] const RidgeFacet<Facet>& f1() const
        {
                return facet_1_;
        }
};

template <typename Facet>
class RidgeFacets final
{
        std::vector<RidgeFacet<Facet>> data_;

public:
        RidgeFacets(const Facet* const facet, const int external_point_index)
        {
                add(facet, external_point_index);
        }

        void add(const Facet* const facet, const int external_point_index)
        {
                data_.emplace_back(facet, external_point_index);
        }

        void remove(const Facet* const facet)
        {
                for (auto& v : data_)
                {
                        if (v.facet() != facet)
                        {
                                continue;
                        }
                        v = data_.back();
                        data_.pop_back();
                        return;
                }

                error("Remove ridge facet: facet not found in the link. Facet " + to_string(facet->vertices()));
        }

        [[nodiscard]] decltype(auto) cbegin() const
        {
                return data_.cbegin();
        }

        [[nodiscard]] decltype(auto) cend() const
        {
                return data_.cend();
        }

        [[nodiscard]] decltype(auto) empty() const
        {
                return data_.empty();
        }

        [[nodiscard]] decltype(auto) size() const
        {
                return data_.size();
        }
};

template <std::size_t N, typename Facet, template <typename...> typename Map, template <typename> typename RidgeFacets>
void add_to_ridges(const Facet* const facet, Map<Ridge<N>, RidgeFacets<Facet>>* const map)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                const auto [iter, inserted] =
                        map->try_emplace(Ridge<N>(sort(del_elem(facet->vertices(), i))), facet, i);

                if (!inserted)
                {
                        iter->second.add(facet, i);
                }
        }
}

template <std::size_t N, typename Facet, template <typename...> typename Map, template <typename> typename RidgeFacets>
void remove_from_ridges(const Facet* const facet, Map<Ridge<N>, RidgeFacets<Facet>>* const map)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                const auto iter = map->find(Ridge<N>(sort(del_elem(facet->vertices(), i))));
                ASSERT(iter != map->end());

                iter->second.remove(facet);
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

        for (std::size_t i = 0; i < N; ++i)
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
struct hash<::ns::geometry::core::Ridge<N>> final
{
        [[nodiscard]] static size_t operator()(const ::ns::geometry::core::Ridge<N>& v)
        {
                return v.hash();
        }
};
}
