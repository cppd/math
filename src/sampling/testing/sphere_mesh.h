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
#include <src/com/type/limit.h>
#include <src/geometry/accelerators/object_bvh.h>
#include <src/geometry/shapes/sphere_create.h>
#include <src/geometry/spatial/hyperplane_mesh_simplex.h>
#include <src/progress/progress.h>

#include <array>
#include <vector>

namespace ns::sampling::testing
{
template <std::size_t N, typename T>
class SphereMesh final
{
        struct Sphere final
        {
                std::vector<Vector<N, T>> vertices;
                std::vector<geometry::HyperplaneMeshSimplex<N, T>> facets;

                explicit Sphere(const unsigned facet_min_count)
                {
                        std::vector<std::array<int, N>> sphere_facets;
                        geometry::create_sphere(facet_min_count, &vertices, &sphere_facets);
                        ASSERT(sphere_facets.size() >= facet_min_count);

                        facets.reserve(sphere_facets.size());
                        for (const std::array<int, N>& vertex_indices : sphere_facets)
                        {
                                facets.emplace_back(&vertices, vertex_indices);
                        }
                }

                Sphere(const Sphere&) = delete;
                Sphere(Sphere&&) = delete;
                Sphere& operator=(const Sphere&) = delete;
                Sphere& operator=(Sphere&&) = delete;
        };

        Sphere sphere_;
        geometry::ObjectBvh<N, T, geometry::HyperplaneMeshSimplex<N, T>> bvh_;

public:
        SphereMesh(const unsigned facet_min_count, ProgressRatio* const progress)
                : sphere_(facet_min_count), bvh_(&sphere_.facets, progress)
        {
        }

        unsigned facet_count() const
        {
                return sphere_.facets.size();
        }

        decltype(auto) facet_vertices(unsigned index) const
        {
                ASSERT(index < facet_count());
                return sphere_.facets[index].vertices();
        }

        std::optional<unsigned> intersect(const Ray<N, T>& ray) const
        {
                const std::optional<std::tuple<T, const geometry::HyperplaneMeshSimplex<N, T>*>> v =
                        bvh_.intersect(ray, Limits<T>::max());
                if (v)
                {
                        const std::size_t index = std::get<1>(*v) - sphere_.facets.data();
                        ASSERT(index < facet_count());
                        return index;
                }
                return std::nullopt;
        }
};
}
