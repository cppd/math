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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/geometry/spatial/hyperplane_simplex.h>
#include <src/numerical/orthogonal.h>

#include <array>
#include <vector>

namespace ns::sampling
{
template <std::size_t N, typename T>
class MeshFacet
{
        static std::array<Vector<N, T>, N> vertices_to_array(
                const std::vector<Vector<N, T>>& vertices,
                const std::array<int, N>& v)
        {
                std::array<Vector<N, T>, N> res;
                for (unsigned i = 0; i < N; ++i)
                {
                        res[i] = vertices[v[i]];
                }
                return res;
        }

        static constexpr int VERTEX_COUNT = N;

        // Количество сочетаний по 2 из N
        // N! / ((N - 2)! * 2!) = (N * (N - 1)) / 2
        static constexpr int VERTEX_RIDGE_COUNT = (N * (N - 1)) / 2;

        const std::vector<Vector<N, T>>& m_vertices;
        const std::array<int, N> m_v;
        Vector<N, T> m_normal;
        geometry::HyperplaneSimplex<N, T> m_geometry;

protected:
        ~MeshFacet() = default;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N - 1;

        using DataType = T;

        MeshFacet(const std::vector<Vector<N, T>>& vertices, const std::array<int, N>& vertex_indices)
                : m_vertices(vertices), m_v(vertex_indices)
        {
                m_normal = numerical::ortho_nn(m_vertices, m_v).normalized();
                if (!is_finite(m_normal))
                {
                        error("Facet normal is not finite, facet vertices\n"
                              + to_string(vertices_to_array(m_vertices, m_v)));
                }

                m_geometry.set_data(m_normal, vertices_to_array(m_vertices, m_v));
        }

        std::optional<T> intersect(const Ray<N, T>& r) const
        {
                return m_geometry.intersect(r, m_vertices[m_v[0]], m_normal);
        }

        std::array<Vector<N, T>, N> vertices() const
        {
                return vertices_to_array(m_vertices, m_v);
        }

        geometry::Constraints<N, T, N, 1> constraints() const
        {
                return m_geometry.constraints(m_normal, vertices_to_array(m_vertices, m_v));
        }

        std::array<std::array<Vector<N, T>, 2>, VERTEX_RIDGE_COUNT> vertex_ridges() const
        {
                std::array<std::array<Vector<N, T>, 2>, VERTEX_RIDGE_COUNT> result;
                unsigned n = 0;
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        for (unsigned j = i + 1; j < N; ++j)
                        {
                                result[n++] = {m_vertices[m_v[i]], m_vertices[m_v[j]] - m_vertices[m_v[i]]};
                        }
                }
                ASSERT(n == VERTEX_RIDGE_COUNT);
                return result;
        }
};
}
