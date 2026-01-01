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

#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/geometry/accelerators/bvh.h>
#include <src/geometry/accelerators/bvh_object.h>
#include <src/geometry/shapes/sphere_create.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/geometry/spatial/hyperplane_simplex.h>
#include <src/geometry/spatial/ray_intersection.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>

#include <array>
#include <cstddef>
#include <optional>
#include <tuple>
#include <vector>

namespace ns::sampling::testing
{
namespace sphere_mesh_implementation
{
template <std::size_t N, typename T>
std::array<numerical::Vector<N, T>, N> vertices_to_array(
        const std::vector<numerical::Vector<N, T>>& vertices,
        const std::array<int, N>& indices)
{
        std::array<numerical::Vector<N, T>, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = vertices[indices[i]];
        }
        return res;
}

template <std::size_t N, typename T>
class Sphere final
{
        std::vector<numerical::Vector<N, T>> vertices_;
        std::vector<std::array<int, N>> facets_;
        std::vector<geometry::spatial::HyperplaneSimplex<N, T>> simplices_;

public:
        explicit Sphere(const unsigned facet_min_count)
        {
                geometry::shapes::create_sphere(facet_min_count, &vertices_, &facets_);
                ASSERT(facets_.size() >= facet_min_count);

                simplices_.reserve(facets_.size());
                for (const std::array<int, N>& indices : facets_)
                {
                        simplices_.emplace_back(vertices_to_array(vertices_, indices));
                }
        }

        [[nodiscard]] const std::vector<geometry::spatial::HyperplaneSimplex<N, T>>& simplices() const
        {
                return simplices_;
        }

        [[nodiscard]] std::array<numerical::Vector<N, T>, N> facet_vertices(const std::size_t index) const
        {
                ASSERT(index < facets_.size());
                return vertices_to_array(vertices_, facets_[index]);
        }

        [[nodiscard]] std::vector<geometry::accelerators::BvhObject<N, T>> bvh_objects() const
        {
                ASSERT(facets_.size() == simplices_.size());
                const auto intersection_cost = decltype(simplices_)::value_type::intersection_cost();
                std::vector<geometry::accelerators::BvhObject<N, T>> res;
                res.reserve(simplices_.size());
                for (std::size_t i = 0; i < simplices_.size(); ++i)
                {
                        res.emplace_back(geometry::spatial::BoundingBox(vertices_, facets_[i]), intersection_cost, i);
                }
                return res;
        }
};
}

template <std::size_t N, typename T>
class SphereMesh final
{
        sphere_mesh_implementation::Sphere<N, T> sphere_;
        geometry::accelerators::Bvh<N, T> bvh_;

public:
        SphereMesh(const unsigned facet_min_count, progress::Ratio* const progress)
                : sphere_(facet_min_count),
                  bvh_(sphere_.bvh_objects(), progress)
        {
        }

        [[nodiscard]] decltype(auto) facet_count() const
        {
                return sphere_.simplices().size();
        }

        [[nodiscard]] decltype(auto) facet_vertices(const std::size_t index) const
        {
                return sphere_.facet_vertices(index);
        }

        [[nodiscard]] std::optional<unsigned> intersect(const numerical::Ray<N, T>& ray) const
        {
                const auto intersection = bvh_.intersect(
                        ray, Limits<T>::max(),
                        [facets = &sphere_.simplices(), &ray](const auto& indices, const auto& max_distance)
                                -> std::optional<std::tuple<T, const geometry::spatial::HyperplaneSimplex<N, T>*>>
                        {
                                const std::tuple<T, const geometry::spatial::HyperplaneSimplex<N, T>*> info =
                                        geometry::spatial::ray_intersection(*facets, indices, ray, max_distance);
                                if (std::get<1>(info))
                                {
                                        return info;
                                }
                                return std::nullopt;
                        });
                if (intersection)
                {
                        const std::size_t index = std::get<1>(*intersection) - sphere_.simplices().data();
                        ASSERT(index < facet_count());
                        return index;
                }
                return std::nullopt;
        }
};
}
