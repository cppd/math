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

#include "parallelotope_aa.h"
#include "ray_intersection.h"
#include "shape_intersection.h"
#include "tree.h"

#include <src/com/thread.h>

#include <optional>
#include <tuple>
#include <vector>

namespace ns::geometry
{
template <typename Object>
class ObjectTree final
{
        static constexpr std::size_t N = Object::SPACE_DIMENSION;
        using T = typename Object::DataType;

        using TreeParallelotope = ParallelotopeAA<N, T>;
        using Tree = SpatialSubdivisionTree<TreeParallelotope>;

        class Intersections final : public Tree::ObjectIntersections
        {
                std::vector<ShapeIntersection<Object>> wrappers_;

                std::vector<int> indices(const TreeParallelotope& parallelotope, const std::vector<int>& indices)
                        const override
                {
                        ShapeIntersection p(&parallelotope);
                        std::vector<int> intersections;
                        intersections.reserve(indices.size());
                        for (int object_index : indices)
                        {
                                if (shape_intersection(p, wrappers_[object_index]))
                                {
                                        intersections.push_back(object_index);
                                }
                        }
                        return intersections;
                }

        public:
                explicit Intersections(const std::vector<Object>& objects)
                {
                        wrappers_.reserve(objects.size());
                        for (const Object& t : objects)
                        {
                                wrappers_.emplace_back(&t);
                        }
                }
        };

        const std::vector<Object>* const objects_;
        Tree tree_;

public:
        ObjectTree(
                const std::vector<Object>* const objects,
                const BoundingBox<N, T>& bounding_box,
                const int min_objects_per_box,
                ProgressRatio* const progress)
                : objects_(objects),
                  tree_(min_objects_per_box,
                        objects->size(),
                        bounding_box,
                        Intersections(*objects),
                        hardware_concurrency(),
                        progress)
        {
        }

        std::optional<T> intersect_root(const Ray<N, T>& ray) const
        {
                return tree_.intersect_root(ray);
        }

        std::optional<std::tuple<T, const Object*>> intersect(const Ray<N, T>& ray, const T& root_distance) const
        {
                struct Info
                {
                        Vector<N, T> point;
                        std::optional<std::tuple<T, const Object*>> intersection;
                        explicit Info(std::optional<std::tuple<T, const Object*>>&& intersection)
                                : intersection(intersection)
                        {
                        }
                };

                const auto f = [&](const std::vector<int>& object_indices) -> std::optional<Info>
                {
                        Info info(ray_intersection(*objects_, object_indices, ray));
                        if (info.intersection)
                        {
                                info.point = ray.point(std::get<0>(*info.intersection));
                                return info;
                        }
                        return std::nullopt;
                };

                const auto info = tree_.trace_ray(ray, root_distance, f);
                if (info)
                {
                        return info->intersection;
                }
                return std::nullopt;
        }
};
}
