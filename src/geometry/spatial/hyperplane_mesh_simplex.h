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
#include "constraint.h"
#include "hyperplane_simplex.h"
#include "parallelotope_aa.h"
#include "shape_overlap.h"

#include "testing/hyperplane_simplex_intersection.h"

#include <src/com/combinatorics.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <array>
#include <vector>

namespace ns::geometry
{
template <std::size_t N, typename T>
class HyperplaneMeshSimplex
{
        static_assert(N >= 3);

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

        static constexpr int EDGE_COUNT = BINOMIAL<N, 2>;

        const std::vector<Vector<N, T>>* vertices_;
        Vector<N, T> plane_n_;
        T plane_d_;
        HyperplaneSimplex<N, T> geometry_;
        std::array<int, N> v_;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N - 1;

        using DataType = T;

        static T intersection_cost()
        {
                return spatial::testing::hyperplane_simplex::intersection_cost<N, T>();
        }

        HyperplaneMeshSimplex(const std::vector<Vector<N, T>>* const vertices, const std::array<int, N>& vertex_indices)
                : vertices_(vertices),
                  plane_n_(numerical::orthogonal_complement(*vertices, vertex_indices).normalized()),
                  plane_d_(dot(plane_n_, (*vertices)[vertex_indices[0]])),
                  v_(vertex_indices)
        {
                if (!is_finite(plane_n_))
                {
                        error("Hyperplane mesh simplex normal " + to_string(plane_n_) + " is not finite, vertices "
                              + to_string(vertices_to_array(*vertices_, v_)));
                }
                geometry_.set_data(plane_n_, vertices_to_array(*vertices_, v_));
        }

        void reverse_normal()
        {
                plane_n_ = -plane_n_;
                plane_d_ = -plane_d_;
        }

        template <std::size_t M>
        Vector<M, T> interpolate(const Vector<N, T>& point, const std::array<Vector<M, T>, N>& data) const
        {
                return geometry_.interpolate(point, data);
        }

        std::optional<T> intersect(const Ray<N, T>& r) const
        {
                return geometry_.intersect(r, plane_n_, plane_d_);
        }

        const Vector<N, T>& normal() const
        {
                return plane_n_;
        }

        std::array<Vector<N, T>, N> vertices() const
        {
                return vertices_to_array(*vertices_, v_);
        }

        Constraints<N, T, N, 1> constraints() const
        {
                return geometry_.constraints(plane_n_, vertices_to_array(*vertices_, v_));
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
                                result[n++] = {(*vertices_)[v_[i]], (*vertices_)[v_[j]] - (*vertices_)[v_[i]]};
                        }
                }
                ASSERT(n == EDGE_COUNT);
                return result;
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
