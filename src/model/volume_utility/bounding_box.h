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

#include "vertices.h"

#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/model/volume.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <cstddef>
#include <optional>

namespace ns::model::volume
{
template <std::size_t N>
struct BoundingBox final
{
        Vector<N, double> min;
        Vector<N, double> max;
};

namespace bounding_box_implementation
{
template <std::size_t N, typename T>
bool min_max_found(const Vector<N, T>& min, const Vector<N, T>& max)
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (!std::isfinite(min[i]))
                {
                        error("Volume min is not finite");
                }
                if (!std::isfinite(max[i]))
                {
                        error("Volume max is not finite");
                }
                if (!(min[i] <= max[i]))
                {
                        return false;
                }
        }
        return true;
}
}

template <std::size_t N>
std::optional<BoundingBox<N>> bounding_box(const Volume<N>& volume)
{
        namespace impl = bounding_box_implementation;
        using T = double;

        Vector<N, T> min = Vector<N, T>(Limits<T>::infinity());
        Vector<N, T> max = Vector<N, T>(-Limits<T>::infinity());

        for (const Vector<N, T>& v : vertices(volume))
        {
                min = ::ns::min(min, v);
                max = ::ns::max(max, v);
        }

        if (!impl::min_max_found(min, max))
        {
                return std::nullopt;
        }

        return {
                {.min = min, .max = max}
        };
}
}
