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

#include <src/com/type/limit.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <optional>

namespace ns::painter
{
template <std::size_t N, typename T>
std::optional<T> hyperplane_intersect(
        const Ray<N, T>& ray,
        const Vector<N, T>& plane_point,
        const Vector<N, T>& plane_normal)
{
        T s = dot(plane_normal, ray.dir());
        T t = dot(plane_point - ray.org(), plane_normal) / s;

        if (t > T(0) && t <= limits<T>::max())
        {
                return t;
        }
        return std::nullopt;
}
}
