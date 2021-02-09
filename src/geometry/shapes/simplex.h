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

#include <src/numerical/complement.h>
#include <src/numerical/vec.h>

#include <cmath>

namespace ns::geometry
{
template <std::size_t N, typename T>
std::array<Vector<N, T>, N + 1> create_origin_centered_simplex()
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        Vector<N + 1, T> simplex_hyperplane_normal = Vector<N + 1, T>(1 / std::sqrt(T(N + 1)));

        std::array<Vector<N + 1, T>, N> basis =
                numerical::orthogonal_complement_of_unit_vector(simplex_hyperplane_normal);

        std::array<Vector<N + 1, T>, N + 1> vertices;
        for (unsigned i = 0; i < N + 1; ++i)
        {
                vertices[i] = Vector<N + 1, T>(0);
        }
        for (unsigned i = 0; i < N + 1; ++i)
        {
                vertices[i][i] = 1;
        }

        std::array<Vector<N, T>, N + 1> result;
        for (unsigned i = 0; i < N + 1; ++i)
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        result[i][n] = dot(vertices[i], basis[n]);
                }
                result[i].normalize();
        }
        return result;
}
}
