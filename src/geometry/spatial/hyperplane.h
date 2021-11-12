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

#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <optional>

namespace ns::geometry
{
template <std::size_t N, typename T>
std::optional<T> hyperplane_intersect(const Ray<N, T>& ray, const Vector<N, T>& plane_n, const T& plane_d)
{
        const T t = (plane_d - dot(plane_n, ray.org())) / dot(plane_n, ray.dir());
        if (t > 0)
        {
                return t;
        }
        return std::nullopt;
}

}
