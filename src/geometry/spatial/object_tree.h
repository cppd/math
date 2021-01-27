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
#include "shape_wrapper.h"
#include "tree.h"

#include <optional>
#include <tuple>
#include <vector>

namespace ns::geometry
{
template <std::size_t N, typename T, typename Object>
class ObjectTree final
{
        using TreeParallelotope = ParallelotopeAA<N, T>;

        static void create_tree(
                int min_objects_per_box,
                const std::vector<Object>& objects,
                const BoundingBox<N, T>& bounding_box,
                SpatialSubdivisionTree<TreeParallelotope>* tree,
                ProgressRatio* progress)
        {
                progress->set_text(to_string(1 << N) + "-tree: %v of %m");

                std::vector<ShapeWrapperForIntersection<Object>> wrappers;
                wrappers.reserve(objects.size());
                for (const Object& t : objects)
                {
                        wrappers.emplace_back(t);
                }

                const auto object_intersections =
                        [w = std::as_const(wrappers)](
                                const TreeParallelotope& parallelotope, const std::vector<int>& indices)
                {
                        ShapeWrapperForIntersection p(parallelotope);
                        std::vector<int> intersections;
                        intersections.reserve(indices.size());
                        for (int object_index : indices)
                        {
                                if (shape_intersection(p, w[object_index]))
                                {
                                        intersections.push_back(object_index);
                                }
                        }
                        return intersections;
                };

                const unsigned thread_count = hardware_concurrency();

                tree->decompose(
                        tree_max_depth<N>(), min_objects_per_box, objects.size(), bounding_box, object_intersections,
                        thread_count, progress);
        }

        static std::optional<std::tuple<T, Object*>> ray_intersection(
                const std::vector<Object>& objects,
                const std::vector<int>& indices,
                const Ray<N, T>& ray)
        {
                T min = limits<T>::max();
                const Object* object = nullptr;

                for (int index : indices)
                {
                        std::optional<T> distance = objects[index].intersect(ray);
                        if (distance && *distance < min)
                        {
                                min = *distance;
                                object = &objects[index];
                        }
                }

                if (object)
                {
                        std::optional<std::tuple<T, Object*>> result(std::in_place);
                        std::get<0>(result) = min;
                        std::get<1>(result) = object;
                        return result;
                }

                return std::nullopt;
        }

        const std::vector<Object>& m_objects;
        SpatialSubdivisionTree<TreeParallelotope> m_tree;

public:
        ObjectTree(const std::vector<Object>& objects, int min_objects_per_box, ProgressRatio* progress)
                : m_objects(objects)
        {
                BoundingBox<N, T> bounding_box;
                bounding_box.min = Vector<N, T>(limits<T>::max());
                bounding_box.max = Vector<N, T>(limits<T>::lowest());

                for (const Object& object : objects)
                {
                        for (const Vector<N, T>& v : object.vertices())
                        {
                                bounding_box.min = min_vector(bounding_box.min, v);
                                bounding_box.max = max_vector(bounding_box.max, v);
                        }
                }

                create_tree(min_objects_per_box, m_objects, bounding_box, &m_tree, progress);
        }

        ObjectTree(const ObjectTree&) = delete;
        ObjectTree(ObjectTree&&) = delete;
        ObjectTree& operator=(const ObjectTree&) = delete;
        ObjectTree& operator=(ObjectTree&&) = delete;

        std::optional<T> intersect_root(const Ray<N, T>& ray) const
        {
                return m_tree.intersect_root(ray);
        }

        std::optional<std::tuple<T, Object*>> intersect(const Ray<N, T>& ray, T root_distance) const
        {
                std::optional<std::tuple<T, Object*>> intersection;

                auto f = [&](const std::vector<int>& object_indices) -> std::optional<Vector<N, T>>
                {
                        intersection = ray_intersection(m_objects, object_indices, ray);
                        if (intersection)
                        {
                                return ray.point(std::get<0>(*intersection));
                        }
                        return std::nullopt;
                };

                if (m_tree.trace_ray(ray, root_distance, f))
                {
                        return intersection;
                }

                return std::nullopt;
        }
};
}
