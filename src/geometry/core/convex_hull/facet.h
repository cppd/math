/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "facet_ortho.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/sort.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <list>
#include <vector>

namespace ns::geometry::core::convex_hull
{
template <typename F>
using FacetList = std::list<F>;

template <typename F>
using FacetListIter = FacetList<F>::const_iterator;

template <std::size_t N, typename DataType, typename ComputeType>
class Facet final
{
        std::array<int, N> vertices_;
        FacetOrtho<N, DataType, ComputeType> ortho_;
        std::vector<int> conflict_points_;
        FacetListIter<Facet> facet_iter_;
        std::array<Facet*, N> links_;
        mutable bool marked_as_visible_ = false;

public:
        Facet(const std::vector<Vector<N, DataType>>& points,
              std::array<int, N>&& vertices,
              const int direction_point,
              const Facet& direction_facet)
                : vertices_(sort(std::move(vertices))),
                  ortho_(points, vertices_, direction_point, direction_facet.ortho_)
        {
        }

        Facet(const std::vector<Vector<N, DataType>>& points, std::array<int, N>&& vertices, const int direction_point)
                : vertices_(sort(std::move(vertices))),
                  ortho_(points, vertices_, direction_point)
        {
        }

        [[nodiscard]] unsigned find_index_for_point(const int point) const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (vertices_[i] == point)
                        {
                                return i;
                        }
                }
                error("Local index not found for point " + to_string(point));
        }

        void add_conflict_point(const int point)
        {
                conflict_points_.push_back(point);
        }

        [[nodiscard]] const std::vector<int>& conflict_points() const
        {
                return conflict_points_;
        }

        void set_iter(FacetListIter<Facet> iter)
        {
                facet_iter_ = std::move(iter);
        }

        [[nodiscard]] FacetListIter<Facet> iter() const
        {
                return facet_iter_;
        }

        void set_link(const unsigned index, Facet* const facet)
        {
                ASSERT(index < N);
                links_[index] = facet;
        }

        [[nodiscard]] Facet* link(const unsigned index) const
        {
                ASSERT(index < N);
                return links_[index];
        }

        [[nodiscard]] unsigned find_link_index(const Facet* const facet) const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (links_[i] == facet)
                        {
                                return i;
                        }
                }
                error("Link index not found for facet");
        }

        void mark_as_visible() const
        {
                marked_as_visible_ = true;
        }

        [[nodiscard]] bool marked_as_visible() const
        {
                return marked_as_visible_;
        }

        [[nodiscard]] const std::array<int, N>& vertices() const
        {
                return vertices_;
        }

        [[nodiscard]] decltype(auto) visible_from_point(
                const std::vector<Vector<N, DataType>>& points,
                const int point_index) const
        {
                // strictly greater than 0
                return ortho_.dot_product_sign(points, vertices_[0], point_index) > 0;
        }

        template <typename T>
        [[nodiscard]] decltype(auto) ortho_fp() const
        {
                return ortho_.template to_floating_point<T>();
        }

        [[nodiscard]] decltype(auto) last_ortho_coord_is_negative() const
        {
                return ortho_.last_coord_is_negative();
        }
};
}
