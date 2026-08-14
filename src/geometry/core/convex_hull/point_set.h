/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <cstddef>
#include <vector>

namespace ns::geometry::core::convex_hull
{
class PointSet final
{
        std::vector<signed char> points_;

public:
        PointSet(const std::size_t size)
                : points_(size, 0)
        {
        }

        void set(const std::vector<int>& points)
        {
                for (const int p : points)
                {
                        points_[p] = 1;
                }
        }

        void clear(const std::vector<int>& points)
        {
                for (const int p : points)
                {
                        points_[p] = 0;
                }
        }

        [[nodiscard]] bool contains(const int point) const
        {
                return points_[point] != 0;
        }
};
}
