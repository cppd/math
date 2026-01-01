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

/*
Satyan L. Devadoss, Joseph O’Rourke.
Discrete and computational geometry.
Princeton University Press, 2011.

4.1 VORONOI GEOMETRY
*/

/*
Voronoi vertex is the center of the sphere that passes
through the vertices of the Delaunay object.
The sphere center is the intersection of the perpendicular
bisectors.

Plane equation
(x - p) ⋅ n = 0
x ⋅ n = p ⋅ n

Plane equation for vertices v(0) and v(n), n >= 1, n <= N
n = v(n) - v(0)
p = (v(n) + v(0)) / 2
x ⋅ (v(n) - v(0)) = ((v(n) + v(0)) / 2) ⋅ (v(n) - v(0))
x ⋅ (2 * (v(n) - v(0))) = v(n) ⋅ v(n) - v(0) ⋅ v(0)
*/

#pragma once

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <vector>

namespace ns::geometry::core
{
template <std::size_t N, typename T>
[[nodiscard]] numerical::Vector<N, T> compute_voronoi_vertex_for_delaunay_object(
        const std::vector<numerical::Vector<N, T>>& points,
        const std::array<int, N + 1>& vertices)
{
        const numerical::Vector<N, T>& p0 = points[vertices[0]];
        const T dot0 = dot(p0, p0);

        numerical::Matrix<N, N, T> a;
        numerical::Vector<N, T> b;

        for (std::size_t row = 0; row < N; ++row)
        {
                const numerical::Vector<N, T>& p = points[vertices[row + 1]];
                for (std::size_t col = 0; col < N; ++col)
                {
                        a[row, col] = 2 * (p[col] - p0[col]);
                }
                b[row] = dot(p, p) - dot0;
        }

        numerical::Vector<N, T> voronoi_vertex = a.solve(b);

        ASSERT(is_finite(voronoi_vertex));

        return voronoi_vertex;
}
}
