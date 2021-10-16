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

namespace ns::geometry
{
template <std::size_t N, typename T>
class BoundingBox final
{
        Vector<N, T> min_;
        Vector<N, T> max_;

public:
        BoundingBox()
        {
        }

        BoundingBox(const Vector<N, T>& min, const Vector<N, T>& max) : min_(min), max_(max)
        {
        }

        explicit BoundingBox(const Vector<N, T>& point) : min_(point), max_(point)
        {
        }

        template <std::size_t SIZE>
        explicit BoundingBox(const std::array<Vector<N, T>, SIZE>& points) : BoundingBox(points[0])
        {
                static_assert(SIZE > 0);
                for (std::size_t i = 1; i < SIZE; ++i)
                {
                        merge(points[i]);
                }
        }

        const Vector<N, T>& min() const
        {
                return min_;
        }

        const Vector<N, T>& max() const
        {
                return max_;
        }

        void merge(const BoundingBox<N, T>& v)
        {
                min_ = ::ns::min(min_, v.min_);
                max_ = ::ns::max(max_, v.max_);
        }

        void merge(const Vector<N, T>& v)
        {
                min_ = ::ns::min(min_, v);
                max_ = ::ns::max(max_, v);
        }
};
}
