/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/geometry/spatial/bounding_box.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::geometry::accelerators
{
template <std::size_t N, typename T>
class BvhObject final
{
        spatial::BoundingBox<N, T> bounds_;
        numerical::Vector<N, T> center_;
        T intersection_cost_;
        unsigned index_;

public:
        BvhObject(const spatial::BoundingBox<N, T>& bounds, const T intersection_cost, const unsigned index)
                : bounds_(bounds),
                  center_(bounds.center()),
                  intersection_cost_(intersection_cost),
                  index_(index)
        {
        }

        [[nodiscard]] const spatial::BoundingBox<N, T>& bounds() const
        {
                return bounds_;
        }

        [[nodiscard]] const numerical::Vector<N, T>& center() const
        {
                return center_;
        }

        [[nodiscard]] T intersection_cost() const
        {
                return intersection_cost_;
        }

        [[nodiscard]] unsigned index() const
        {
                return index_;
        }
};
}
