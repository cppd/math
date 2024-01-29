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

#include "hyperplane.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>

namespace ns::geometry::spatial
{
template <std::size_t N, typename T>
class HyperplaneBall final
{
        Hyperplane<N, T> plane_;
        Vector<N, T> center_;
        T radius_squared_;

public:
        static T intersection_cost();

        HyperplaneBall(const Vector<N, T>& center, const Vector<N, T>& normal, const T& radius)
                : center_(center),
                  radius_squared_(square(radius))
        {
                plane_.n = normal.normalized();
                if (!is_finite(plane_.n))
                {
                        error("Hyperplane ball normal " + to_string(plane_.n) + " is not finite");
                }
                plane_.d = dot(plane_.n, center_);
        }

        [[nodiscard]] std::optional<T> intersect(const numerical::Ray<N, T>& ray) const
        {
                const T t = plane_.intersect(ray);
                if (!(t > 0))
                {
                        return std::nullopt;
                }

                const Vector<N, T> point = ray.point(t);

                if ((point - center_).norm_squared() < radius_squared_)
                {
                        return t;
                }
                return std::nullopt;
        }

        [[nodiscard]] const Vector<N, T>& center() const
        {
                return center_;
        }

        [[nodiscard]] const Vector<N, T>& normal() const
        {
                return plane_.n;
        }

        [[nodiscard]] const T& radius_squared() const
        {
                return radius_squared_;
        }
};
}
