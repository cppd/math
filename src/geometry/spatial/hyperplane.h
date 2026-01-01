/*
Copyright (C) 2017-2026 Topological Manifold

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
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::geometry::spatial
{
template <std::size_t N, typename T>
struct Hyperplane final
{
        // n * x - d = 0
        numerical::Vector<N, T> n;
        T d;

        constexpr Hyperplane()
        {
        }

        constexpr Hyperplane(const numerical::Vector<N, T>& n, const T& d)
                : n(n),
                  d(d)
        {
        }

        // equation n * x + d = 0
        explicit constexpr Hyperplane(const numerical::Vector<N + 1, T>& equation)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        n[i] = equation[i];
                }
                d = -equation[N];
        }

        constexpr void reverse_normal()
        {
                n = -n;
                d = -d;
        }

        [[nodiscard]] T intersect(const numerical::Ray<N, T>& ray) const
        {
                return (d - dot(n, ray.org())) / dot(n, ray.dir());
        }

        [[nodiscard]] T distance(const numerical::Vector<N, T>& point) const
        {
                return dot(n, point) - d;
        }

        [[nodiscard]] numerical::Vector<N, T> project(const numerical::Vector<N, T>& point) const
        {
                return point - n * distance(point);
        }
};
}
