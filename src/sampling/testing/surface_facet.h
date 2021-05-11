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
#include <src/geometry/shapes/sphere_area.h>
#include <src/geometry/shapes/sphere_simplex.h>
#include <src/geometry/spatial/hyperplane_simplex.h>
#include <src/numerical/complement.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <vector>

namespace ns::sampling::testing
{
template <std::size_t N, typename T>
class SurfaceFacet final
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
        static constexpr int EDGE_COUNT = (N * (N - 1)) / 2;

        const std::vector<Vector<N, T>>& m_vertices;
        const std::array<int, N> m_v;
        Vector<N, T> m_normal;
        geometry::HyperplaneSimplex<N, T> m_geometry;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N - 1;

        using DataType = T;

        SurfaceFacet(const std::vector<Vector<N, T>>& vertices, const std::array<int, N>& vertex_indices)
                : m_vertices(vertices), m_v(vertex_indices)
        {
                m_normal = numerical::orthogonal_complement(m_vertices, m_v).normalized();
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

        std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> edges() const
        {
                static_assert(N <= 3);

                std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> result;
                unsigned n = 0;
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        for (unsigned j = i + 1; j < N; ++j)
                        {
                                result[n++] = {m_vertices[m_v[i]], m_vertices[m_v[j]] - m_vertices[m_v[i]]};
                        }
                }
                ASSERT(n == EDGE_COUNT);
                return result;
        }
};

template <std::size_t N, typename T>
double surface_facet_area(
        const SurfaceFacet<N, T>& facet,
        const long long uniform_count,
        const long long all_uniform_count)
{
        static constexpr double SPHERE_AREA = geometry::sphere_area(N);

        double area = double(uniform_count) / all_uniform_count * SPHERE_AREA;
        if constexpr (N == 3)
        {
                const double geometry_area = geometry::sphere_simplex_area(facet.vertices());

                const double relative_error = std::abs(area - geometry_area) / std::max(geometry_area, area);
                if (!(relative_error < 0.025))
                {
                        std::ostringstream oss;
                        oss << "bucket area relative error = " << relative_error << '\n';
                        oss << "bucket area = " << area << '\n';
                        oss << "geometry bucket area = " << geometry_area << '\n';
                        oss << "uniform count = " << uniform_count << '\n';
                        oss << "all uniform count = " << all_uniform_count;
                        error(oss.str());
                }

                area = geometry_area;
        }
        return area;
}
}
