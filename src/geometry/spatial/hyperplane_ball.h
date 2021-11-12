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

#include "hyperplane.h"

#include <src/com/exponent.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <optional>

namespace ns::geometry
{
template <std::size_t N, typename T>
class HyperplaneBall final
{
        Vector<N, T> center_;
        Vector<N, T> plane_n_;
        T plane_d_;
        T radius_squared_;

public:
        HyperplaneBall(const Vector<N, T>& center, const Vector<N, T>& normal, const T& radius)
                : center_(center),
                  plane_n_(normal.normalized()),
                  plane_d_(dot(plane_n_, center_)),
                  radius_squared_(square(radius))
        {
        }

        std::optional<T> intersect(const Ray<N, T>& ray) const
        {
                const std::optional<T> t = hyperplane_intersect(ray, plane_n_, plane_d_);
                if (!t)
                {
                        return std::nullopt;
                }

                const Vector<N, T> point = ray.point(*t);

                if ((point - center_).norm_squared() < radius_squared_)
                {
                        return t;
                }
                return std::nullopt;
        }

        const Vector<N, T>& center() const
        {
                return center_;
        }

        const Vector<N, T>& normal() const
        {
                return plane_n_;
        }

        const T& radius_squared() const
        {
                return radius_squared_;
        }
};
}
