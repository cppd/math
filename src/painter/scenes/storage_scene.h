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

#include "../objects.h"
#include "../shapes/shape.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/math.h>
#include <src/com/type/limit.h>
#include <src/geometry/spatial/parallelotope_aa.h>
#include <src/geometry/spatial/shape_intersection.h>
#include <src/geometry/spatial/shape_wrapper.h>
#include <src/geometry/spatial/tree.h>

#include <algorithm>
#include <memory>
#include <tuple>
#include <vector>

namespace ns::painter
{
namespace scene_implementation
{
constexpr int TREE_MIN_OBJECTS_PER_BOX = 10;

template <std::size_t N>
int tree_max_depth()
{
        static_assert(N >= 3);

        switch (N)
        {
        case 3:
                return 10;
        case 4:
                return 8;
        case 5:
                return 6;
        case 6:
                return 5;
        default:
                // Сумма геометрической прогрессии s = (pow(r, n) - 1) / (r - 1).
                // Для s и r найти n = log(s * (r - 1) + 1) / log(r).
                double s = 1e9;
                double r = std::pow(2, N);
                double n = std::log(s * (r - 1) + 1) / std::log(r);
                return std::max(2.0, std::floor(n));
        }
}

template <typename P>
std::vector<P*> to_pointers(const std::vector<std::unique_ptr<P>>& objects)
{
        std::vector<P*> result;
        result.reserve(objects.size());
        for (const std::unique_ptr<P>& p : objects)
        {
                ASSERT(p);
                result.push_back(p.get());
        }
        return result;
}

template <std::size_t N, typename T>
geometry::BoundingBox<N, T> compute_bounding_box(const std::vector<const Shape<N, T>*>& shapes)
{
        geometry::BoundingBox<N, T> bb = shapes[0]->bounding_box();
        for (std::size_t i = 1; i < shapes.size(); ++i)
        {
                geometry::BoundingBox<N, T> shape_bb = shapes[i]->bounding_box();
                bb.min = min_vector(bb.min, shape_bb.min);
                bb.max = max_vector(bb.max, shape_bb.max);
        }
        return bb;
}

template <std::size_t N, typename T>
struct BoundingIntersection
{
        T distance;
        const Shape<N, T>* shape;

        BoundingIntersection(T distance, const Shape<N, T>* shape) : distance(distance), shape(shape)
        {
        }

        bool operator<(const BoundingIntersection& b) const
        {
                return distance < b.distance;
        }
};

template <std::size_t N, typename T>
std::optional<Intersection<N, T>> ray_intersect(
        const std::vector<const Shape<N, T>*>& shapes,
        const std::vector<int>& indices,
        const Ray<N, T>& ray)
{
        if (indices.size() == 1)
        {
                std::optional<T> distance = shapes[indices.front()]->intersect_bounding(ray);
                if (distance)
                {
                        return shapes[indices.front()]->intersect(ray, *distance);
                }
                return std::nullopt;
        }

        // Объекты могут быть сложными, поэтому перед поиском точного пересечения
        // их надо разместить по возрастанию примерного пересечения.

        thread_local std::vector<BoundingIntersection<N, T>> intersections;
        intersections.clear();
        intersections.reserve(indices.size());
        for (int index : indices)
        {
                std::optional<T> distance = shapes[index]->intersect_bounding(ray);
                if (distance)
                {
                        intersections.emplace_back(*distance, shapes[index]);
                }
        }
        if (intersections.empty())
        {
                return std::nullopt;
        }

        std::make_heap(intersections.begin(), intersections.end());

        T min_distance_squared = limits<T>::max();
        std::optional<Intersection<N, T>> intersection;

        do
        {
                const BoundingIntersection<N, T>& bounding = intersections.front();

                if (min_distance_squared < square(bounding.distance))
                {
                        break;
                }

                std::optional<Intersection<N, T>> v = bounding.shape->intersect(ray, bounding.distance);
                if (v)
                {
                        T distance_squared = (v->point - ray.org()).norm_squared();
                        if (distance_squared < min_distance_squared)
                        {
                                min_distance_squared = distance_squared;
                                intersection = v;
                        }
                }

                std::pop_heap(intersections.begin(), intersections.end());
                intersections.pop_back();
        } while (!intersections.empty());

        return intersection;
}

template <std::size_t N, typename T>
void create_tree(
        const std::vector<const Shape<N, T>*>& shapes,
        const geometry::BoundingBox<N, T>& bounding_box,
        geometry::SpatialSubdivisionTree<geometry::ParallelotopeAA<N, T>>* tree,
        ProgressRatio* progress)
{
        std::vector<std::function<bool(const geometry::ShapeWrapperForIntersection<geometry::ParallelotopeAA<N, T>>&)>>
                wrappers;
        wrappers.reserve(shapes.size());
        for (const Shape<N, T>* s : shapes)
        {
                wrappers.push_back(s->intersection_function());
        }

        const auto shape_intersections =
                [&w = std::as_const(wrappers)](
                        const geometry::ParallelotopeAA<N, T>& parallelotope, const std::vector<int>& indices)
        {
                geometry::ShapeWrapperForIntersection p(parallelotope);
                std::vector<int> intersections;
                intersections.reserve(indices.size());
                for (int object_index : indices)
                {
                        if (w[object_index](p))
                        {
                                intersections.push_back(object_index);
                        }
                }
                return intersections;
        };

        const unsigned thread_count = hardware_concurrency();

        tree->decompose(
                tree_max_depth<N>(), TREE_MIN_OBJECTS_PER_BOX, wrappers.size(), bounding_box, shape_intersections,
                thread_count, progress);
}
}

template <std::size_t N, typename T>
class StorageScene final : public Scene<N, T>
{
        inline static thread_local std::int_fast64_t m_thread_ray_count = 0;

        std::vector<std::unique_ptr<const Shape<N, T>>> m_shapes;
        std::vector<std::unique_ptr<const LightSource<N, T>>> m_light_sources;

        std::unique_ptr<const Projector<N, T>> m_projector;

        Color m_background_color;
        Color m_background_light_source_color;

        std::vector<const Shape<N, T>*> m_shape_pointers;
        std::vector<const LightSource<N, T>*> m_light_source_pointers;

        T m_size;

        geometry::SpatialSubdivisionTree<geometry::ParallelotopeAA<N, T>> m_tree;

        T size() const override
        {
                return m_size;
        }

        std::optional<Intersection<N, T>> intersect(const Ray<N, T>& ray) const override
        {
                ++m_thread_ray_count;

                std::optional<T> root = m_tree.intersect_root(ray);
                if (!root)
                {
                        return std::nullopt;
                }

                std::optional<Intersection<N, T>> intersection;

                const auto f = [&](const std::vector<int>& shape_indices) -> std::optional<Vector<N, T>>
                {
                        intersection = scene_implementation::ray_intersect(m_shape_pointers, shape_indices, ray);
                        if (intersection)
                        {
                                return intersection->point;
                        }
                        return std::nullopt;
                };

                if (m_tree.trace_ray(ray, *root, f))
                {
                        return intersection;
                }

                return std::nullopt;
        }

        const std::vector<const LightSource<N, T>*>& light_sources() const override
        {
                return m_light_source_pointers;
        }

        const Projector<N, T>& projector() const override
        {
                return *m_projector;
        }

        const Color& background_color() const override
        {
                return m_background_color;
        }

        const Color& background_light_source_color() const override
        {
                return m_background_light_source_color;
        }

        long long thread_ray_count() const noexcept override
        {
                return m_thread_ray_count;
        }

public:
        StorageScene(
                const Color& background_color,
                std::unique_ptr<const Projector<N, T>>&& projector,
                std::vector<std::unique_ptr<const LightSource<N, T>>>&& light_sources,
                std::vector<std::unique_ptr<const Shape<N, T>>>&& shapes)
                : m_shapes(std::move(shapes)),
                  m_light_sources(std::move(light_sources)),
                  m_projector(std::move(projector)),
                  m_background_color(background_color),
                  m_background_light_source_color(Color(m_background_color.luminance())),
                  m_shape_pointers(scene_implementation::to_pointers(m_shapes)),
                  m_light_source_pointers(scene_implementation::to_pointers(m_light_sources))
        {
                ASSERT(m_projector);

                const geometry::BoundingBox<N, T> bounding_box =
                        scene_implementation::compute_bounding_box(m_shape_pointers);

                m_size = (bounding_box.max - bounding_box.min).norm();

                ProgressRatio progress(nullptr);
                scene_implementation::create_tree(m_shape_pointers, bounding_box, &m_tree, &progress);
        }
};
}
