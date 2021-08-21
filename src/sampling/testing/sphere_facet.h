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

#include <src/com/combinatorics.h>
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
class SphereFacet final
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
        static constexpr int EDGE_COUNT = binomial<N, 2>();

        const std::vector<Vector<N, T>>& vertices_;
        const std::array<int, N> v_;
        Vector<N, T> normal_;
        geometry::HyperplaneSimplex<N, T> geometry_;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N - 1;

        using DataType = T;

        SphereFacet(const std::vector<Vector<N, T>>& vertices, const std::array<int, N>& vertex_indices)
                : vertices_(vertices), v_(vertex_indices)
        {
                normal_ = numerical::orthogonal_complement(vertices_, v_).normalized();
                if (!is_finite(normal_))
                {
                        error("Facet normal is not finite, facet vertices\n"
                              + to_string(vertices_to_array(vertices_, v_)));
                }

                geometry_.set_data(normal_, vertices_to_array(vertices_, v_));
        }

        std::optional<T> intersect(const Ray<N, T>& r) const
        {
                return geometry_.intersect(r, vertices_[v_[0]], normal_);
        }

        std::array<Vector<N, T>, N> vertices() const
        {
                return vertices_to_array(vertices_, v_);
        }

        geometry::Constraints<N, T, N, 1> constraints() const
        {
                return geometry_.constraints(normal_, vertices_to_array(vertices_, v_));
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
                                result[n++] = {vertices_[v_[i]], vertices_[v_[j]] - vertices_[v_[i]]};
                        }
                }
                ASSERT(n == EDGE_COUNT);
                return result;
        }
};

template <std::size_t N, typename T>
double sphere_facet_area(
        const SphereFacet<N, T>& facet,
        const long long uniform_count,
        const long long all_uniform_count)
{
        static constexpr double SPHERE_AREA = geometry::sphere_area<N>();

        double area = double(uniform_count) / all_uniform_count * SPHERE_AREA;
        if constexpr (N == 3)
        {
                const double geometry_area = geometry::sphere_simplex_area(facet.vertices());

                const double relative_error = std::abs(area - geometry_area) / std::max(geometry_area, area);
                if (!(relative_error < 0.025))
                {
                        std::ostringstream oss;
                        oss << "sphere area relative error = " << relative_error << '\n';
                        oss << "sphere area = " << area << '\n';
                        oss << "geometry sphere area = " << geometry_area << '\n';
                        oss << "uniform count = " << uniform_count << '\n';
                        oss << "all uniform count = " << all_uniform_count;
                        error(oss.str());
                }

                area = geometry_area;
        }
        return area;
}
}
