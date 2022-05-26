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

#include "integer_convert.h"
#include "integer_types.h"

#include <src/com/random/pcg.h>
#include <src/com/shuffle.h>

#include <vector>

namespace ns::geometry::convex_hull
{
namespace points_implementation
{
template <std::size_t N>
std::array<int, N> restore_indices(const std::array<int, N>& vertices, const std::vector<int>& map)
{
        std::array<int, N> res;
        for (std::size_t n = 0; n < N; ++n)
        {
                res[n] = map[vertices[n]];
        }
        return res;
}
}

template <std::size_t N>
class ConvexHullPoints
{
        std::vector<int> map_;
        std::vector<Vector<N, ConvexHullSourceInteger>> points_;

public:
        explicit ConvexHullPoints(const std::vector<Vector<N, float>>& source_points)
        {
                convert_to_unique_integer(source_points, MAX_CONVEX_HULL, &points_, &map_);
                shuffle(PCG(points_.size()), &points_, &map_);
        }

        [[nodiscard]] const std::vector<Vector<N, ConvexHullSourceInteger>>& points() const
        {
                return points_;
        }

        template <std::size_t M>
        [[nodiscard]] std::array<int, M> restore_indices(const std::array<int, M>& vertices) const
        {
                return points_implementation::restore_indices(vertices, map_);
        }
};

template <std::size_t N>
class DelaunayPoints
{
        std::vector<int> map_;
        std::vector<Vector<N, DelaunaySourceInteger>> points_;

public:
        explicit DelaunayPoints(const std::vector<Vector<N, float>>& source_points)
        {
                convert_to_unique_integer(source_points, MAX_DELAUNAY, &points_, &map_);
                shuffle(PCG(points_.size()), &points_, &map_);
        }

        [[nodiscard]] const std::vector<Vector<N, DelaunaySourceInteger>>& points() const
        {
                return points_;
        }

        template <std::size_t M>
        [[nodiscard]] std::array<int, M> restore_indices(const std::array<int, M>& vertices) const
        {
                return points_implementation::restore_indices(vertices, map_);
        }

        [[nodiscard]] int restore_index(const int index) const
        {
                return map_[index];
        }
};
}
