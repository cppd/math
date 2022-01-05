/*
Copyright (C) 2017-2022 Topological Manifold

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

namespace ns::geometry
{
template <std::size_t N, typename T>
struct Hyperplane final
{
        // n * x - d = 0
        Vector<N, T> n;
        T d;

        Hyperplane()
        {
        }

        Hyperplane(const Vector<N, T>& n, const T& d) : n(n), d(d)
        {
        }

        void reverse_normal()
        {
                n = -n;
                d = -d;
        }

        [[nodiscard]] T intersect(const Ray<N, T>& ray) const
        {
                return (d - dot(n, ray.org())) / dot(n, ray.dir());
        }

        [[nodiscard]] T distance(const Vector<N, T>& point) const
        {
                return dot(n, point) - d;
        }

        [[nodiscard]] Vector<N, T> project(const Vector<N, T>& point) const
        {
                return point - n * distance(point);
        }
};
}
