/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/numerical/vec.h>

#include <array>

namespace ns::painter
{
template <std::size_t N, typename T>
struct BoundingBox final
{
        Vector<N, T> min;
        Vector<N, T> max;

        BoundingBox() = default;

        BoundingBox(const Vector<N, T>& min, const Vector<N, T>& max) : min(min), max(max)
        {
        }

        template <std::size_t Size>
        explicit BoundingBox(const std::array<Vector<N, T>, Size>& points)
        {
                static_assert(Size > 0);
                min = points[0];
                max = points[0];
                for (std::size_t i = 1; i < Size; ++i)
                {
                        min = min_vector(points[i], min);
                        max = max_vector(points[i], max);
                }
        }
};
}
