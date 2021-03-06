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
#include "shape_intersection.h"
#include "shape_wrapper.h"
#include "tree.h"

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

        static BoundingBox<N, T> bounding_box(const std::vector<Object>& objects)
        {
                BoundingBox<N, T> bb;
                bb.min = Vector<N, T>(limits<T>::max());
                bb.max = Vector<N, T>(limits<T>::lowest());
                for (const Object& object : objects)
                {
                        for (const Vector<N, T>& v : object.vertices())
                        {
                                bb.min = min_vector(bb.min, v);
                                bb.max = max_vector(bb.max, v);
                        }
                }
                return bb;
        }

        static void create_tree(
                int max_depth,
                int min_objects_per_box,
                const std::vector<Object>& objects,
                const BoundingBox<N, T>& bounding_box,
                SpatialSubdivisionTree<TreeParallelotope>* tree,
                ProgressRatio* progress)
        {
                std::vector<ShapeWrapperForIntersection<Object>> wrappers;
                wrappers.reserve(objects.size());
                for (const Object& t : objects)
                {
                        wrappers.emplace_back(t);
                }

                const auto object_intersections =
                        [&w = std::as_const(wrappers)](
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
                        max_depth, min_objects_per_box, objects.size(), bounding_box, object_intersections,
                        thread_count, progress);
        }

        static std::optional<std::tuple<T, const Object*>> ray_intersection(
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
                        return std::make_tuple(min, object);
                }

                return std::nullopt;
        }

        const std::vector<Object>& m_objects;
        BoundingBox<N, T> m_bounding_box;
        SpatialSubdivisionTree<TreeParallelotope> m_tree;

public:
        ObjectTree(const std::vector<Object>& objects, int max_depth, int min_objects_per_box, ProgressRatio* progress)
                : m_objects(objects)
        {
                m_bounding_box = bounding_box(objects);
                create_tree(max_depth, min_objects_per_box, m_objects, m_bounding_box, &m_tree, progress);
        }

        const BoundingBox<N, T>& bounding_box() const
        {
                return m_bounding_box;
        }

        const TreeParallelotope& root() const
        {
                return m_tree.root();
        }

        std::optional<T> intersect_root(const Ray<N, T>& ray) const
        {
                return m_tree.intersect_root(ray);
        }

        std::optional<std::tuple<T, const Object*>> intersect(const Ray<N, T>& ray, T root_distance) const
        {
                std::optional<std::tuple<T, const Object*>> intersection;

                const auto f = [&](const std::vector<int>& object_indices) -> std::optional<Vector<N, T>>
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
