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

#include "hyperplane.h"

#include <src/numerical/ray.h>

#include <cstddef>
#include <vector>

namespace ns::geometry::spatial
{
template <std::size_t N, typename T>
class ConvexPolytope final
{
        static_assert(N >= 1);
        static_assert(std::is_floating_point_v<T>);

        // Planes have vectors n directed outward
        std::vector<Hyperplane<N, T>> planes_;

public:
        explicit ConvexPolytope(std::vector<Hyperplane<N, T>> planes)
                : planes_(std::move(planes))
        {
        }

        [[nodiscard]] bool intersect(const Ray<N, T>& ray, T* const near, T* const far) const
        {
                T l_near = *near;
                T l_far = *far;

                for (const Hyperplane<N, T>& plane : planes_)
                {
                        const T s = dot(ray.dir(), plane.n);
                        const T d = dot(ray.org(), plane.n);

                        if (s == 0)
                        {
                                if (d > plane.d)
                                {
                                        return false;
                                }
                                continue;
                        }

                        const T a = (plane.d - d) / s;
                        if (s > 0)
                        {
                                l_far = a < l_far ? a : l_far;
                        }
                        else
                        {
                                l_near = a > l_near ? a : l_near;
                        }

                        if (l_far < l_near)
                        {
                                return false;
                        }
                }

                *near = l_near;
                *far = l_far;
                return true;
        }
};
}
