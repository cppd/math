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

#include "bounding_box.h"
#include "hyperplane_simplex.h"
#include "parallelotope_aa.h"
#include "shape_overlap.h"

#include <src/com/combinatorics.h>
#include <src/com/error.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <array>
#include <vector>

namespace ns::geometry
{
template <std::size_t N, typename T>
class HyperplaneMeshSimplex
{
        static std::array<Vector<N, T>, N> vertices_to_array(
                const std::vector<Vector<N, T>>& vertices,
                const std::array<int, N>& indices)
        {
                std::array<Vector<N, T>, N> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = vertices[indices[i]];
                }
                return res;
        }

        static constexpr std::size_t EDGE_COUNT = BINOMIAL<N, 2>;

        HyperplaneSimplex<N, T> simplex_;
        const std::vector<Vector<N, T>>* vertices_;
        std::array<int, N> indices_;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N - 1;

        using DataType = T;

        HyperplaneMeshSimplex(const std::vector<Vector<N, T>>* const vertices, const std::array<int, N>& indices)
                : simplex_(vertices_to_array(*vertices, indices)), vertices_(vertices), indices_(indices)
        {
        }

        void reverse_normal()
        {
                simplex_.reverse_normal();
        }

        static decltype(auto) intersection_cost()
        {
                return decltype(simplex_)::intersection_cost();
        }

        template <std::size_t M>
        decltype(auto) interpolate(const Vector<N, T>& point, const std::array<Vector<M, T>, N>& data) const
        {
                return simplex_.interpolate(point, data);
        }

        decltype(auto) intersect(const Ray<N, T>& ray) const
        {
                return simplex_.intersect(ray);
        }

        decltype(auto) normal() const
        {
                return simplex_.normal();
        }

        decltype(auto) project(const Vector<N, T>& point) const
        {
                return simplex_.project(point);
        }

        decltype(auto) constraints() const
        {
                return simplex_.constraints(vertices());
        }

        std::array<Vector<N, T>, N> vertices() const
        {
                return vertices_to_array(*vertices_, indices_);
        }

        std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> edges() const
        {
                static_assert(N <= 3);

                const std::array<Vector<N, T>, N> v = vertices();

                std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> res;
                std::size_t n = 0;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        for (std::size_t j = i + 1; j < N; ++j)
                        {
                                res[n++] = {v[i], v[j] - v[i]};
                        }
                }
                ASSERT(n == EDGE_COUNT);
                return res;
        }

        auto overlap_function() const
        {
                return [s = ShapeOverlap(this)](const ShapeOverlap<ParallelotopeAA<N, T>>& p)
                {
                        return shapes_overlap(s, p);
                };
        }

        BoundingBox<N, T> bounding_box() const
        {
                return BoundingBox<N, T>(vertices());
        }
};
}
