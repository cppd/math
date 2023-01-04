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

#include <src/com/type/limit.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <cmath>

namespace ns::geometry::spatial
{
namespace point_offset_implementation
{
template <typename T>
inline constexpr T OFFSET = 64 * Limits<T>::epsilon();
}

template <std::size_t N, typename T>
[[nodiscard]] Vector<N, T> offset_ray_org(const Vector<N, T>& normal, const Ray<N, T>& ray)
{
        namespace impl = point_offset_implementation;

        const T ray_offset = (dot(normal, ray.dir()) < 0) ? -impl::OFFSET<T> : impl::OFFSET<T>;
        Vector<N, T> org;
        for (std::size_t i = 0; i < N; ++i)
        {
                org[i] = ray.org()[i] + std::abs(ray.org()[i]) * ray_offset * normal[i];
        }
        return org;
}

template <std::size_t N, typename T>
[[nodiscard]] Vector<N, T> offset_point(const Vector<N, T>& normal, const Vector<N, T>& point)
{
        namespace impl = point_offset_implementation;

        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = point[i] + std::abs(point[i]) * impl::OFFSET<T> * normal[i];
        }
        return res;
}
}
