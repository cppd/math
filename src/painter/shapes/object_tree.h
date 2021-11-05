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

#include "ray_intersection.h"

#include <src/com/thread.h>
#include <src/geometry/spatial/parallelotope_aa.h>
#include <src/geometry/spatial/shape_intersection.h>
#include <src/geometry/spatial/tree.h>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
class ObjectTree final
{
        static constexpr int MIN_OBJECTS_PER_BOX = 10;

        using TreeParallelotope = geometry::ParallelotopeAA<N, T>;
        using Tree = geometry::SpatialSubdivisionTree<TreeParallelotope>;

        class Intersections final : public Tree::ObjectIntersections
        {
                std::vector<std::function<bool(const geometry::ShapeIntersection<TreeParallelotope>&)>> wrappers_;

                std::vector<int> indices(const TreeParallelotope& parallelotope, const std::vector<int>& indices)
                        const override
                {
                        geometry::ShapeIntersection p(&parallelotope);
                        std::vector<int> intersections;
                        intersections.reserve(indices.size());
                        for (int object_index : indices)
                        {
                                if (wrappers_[object_index](p))
                                {
                                        intersections.push_back(object_index);
                                }
                        }
                        return intersections;
                }

        public:
                explicit Intersections(const std::vector<const Shape<N, T, Color>*>& objects)
                {
                        wrappers_.reserve(objects.size());
                        for (const Shape<N, T, Color>* s : objects)
                        {
                                wrappers_.push_back(s->intersection_function());
                        }
                }
        };

        const std::vector<const Shape<N, T, Color>*>* const objects_;
        Tree tree_;

public:
        ObjectTree(
                const std::vector<const Shape<N, T, Color>*>* const objects,
                const geometry::BoundingBox<N, T>& bounding_box,
                ProgressRatio* const progress)
                : objects_(objects),
                  tree_(MIN_OBJECTS_PER_BOX,
                        objects->size(),
                        bounding_box,
                        Intersections(*objects),
                        hardware_concurrency(),
                        progress)
        {
        }

        const Surface<N, T, Color>* intersect(const Ray<N, T>& ray) const
        {
                const std::optional<T> root_distance = tree_.intersect_root(ray);
                if (!root_distance)
                {
                        return nullptr;
                }

                struct Info
                {
                        Vector<N, T> point;
                        const Surface<N, T, Color>* surface;
                        explicit Info(const ShapeIntersection<N, T, Color>& intersection)
                                : surface(intersection.surface)
                        {
                        }
                };

                const auto f = [&](const std::vector<int>& shape_indices) -> std::optional<Info>
                {
                        Info info(ray_intersection(*objects_, shape_indices, ray));
                        if (info.surface)
                        {
                                info.point = info.surface->point();
                                return info;
                        }
                        return std::nullopt;
                };

                const auto info = tree_.trace_ray(ray, *root_distance, f);
                return info ? info->surface : nullptr;
        }
};
}
