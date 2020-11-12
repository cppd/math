/*
Copyright (C) 2017-2020 Topological Manifold

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
#include "../space/parallelotope_aa.h"
#include "../space/shape_intersection.h"
#include "../space/shape_wrapper.h"
#include "../space/tree.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <algorithm>
#include <memory>
#include <tuple>
#include <vector>

namespace painter
{
namespace scene_implementation
{
constexpr int TREE_MIN_OBJECTS_PER_BOX = 10;

template <size_t N>
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

template <size_t N, typename T>
BoundingBox<N, T> compute_bounding_box(const std::vector<const Shape<N, T>*>& shapes)
{
        BoundingBox<N, T> bb = shapes[0]->bounding_box();
        for (size_t i = 1; i < shapes.size(); ++i)
        {
                BoundingBox<N, T> shape_bb = shapes[i]->bounding_box();
                bb.min = min_vector(bb.min, shape_bb.min);
                bb.max = max_vector(bb.max, shape_bb.max);
        }
        return bb;
}

template <size_t N, typename T>
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

        std::vector<std::tuple<T, const Shape<N, T>*>> intersections;
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

        std::sort(
                intersections.begin(), intersections.end(),
                [](const std::tuple<T, const Shape<N, T>*>& a, const std::tuple<T, const Shape<N, T>*>& b) {
                        return std::get<0>(a) < std::get<0>(b);
                });

        T min_distance = limits<T>::max();

        std::optional<Intersection<N, T>> intersection;

        for (const auto& [bounding_distance, object] : intersections)
        {
                if (min_distance < bounding_distance)
                {
                        break;
                }

                std::optional<Intersection<N, T>> v = object->intersect(ray, bounding_distance);
                if (v && (v->distance < min_distance))
                {
                        min_distance = v->distance;
                        intersection = v;
                }
        }

        return intersection;
}

template <size_t N, typename T>
void create_tree(
        const std::vector<const Shape<N, T>*>& shapes,
        SpatialSubdivisionTree<ParallelotopeAA<N, T>>* tree,
        ProgressRatio* progress)
{
        BoundingBox<N, T> bounding_box;
        bounding_box.min = Vector<N, T>(limits<T>::max());
        bounding_box.max = Vector<N, T>(limits<T>::lowest());

        std::vector<std::function<bool(const ShapeWrapperForIntersection<ParallelotopeAA<N, T>>&)>> wrappers;
        wrappers.reserve(shapes.size());
        for (const Shape<N, T>* s : shapes)
        {
                wrappers.push_back(s->intersection_function());
                BoundingBox<N, T> shape_bb = s->bounding_box();
                bounding_box.min = min_vector(bounding_box.min, shape_bb.min);
                bounding_box.max = max_vector(bounding_box.max, shape_bb.max);
        }

        const auto shape_intersections = [w = std::as_const(wrappers)](
                                                 const ParallelotopeAA<N, T>& parallelotope,
                                                 const std::vector<int>& indices) {
                ShapeWrapperForIntersection p(parallelotope);
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

template <size_t N, typename T>
class StorageScene final : public Scene<N, T>
{
        std::vector<std::unique_ptr<const Shape<N, T>>> m_shapes;
        std::vector<std::unique_ptr<const LightSource<N, T>>> m_light_sources;

        std::unique_ptr<const Projector<N, T>> m_projector;

        Color m_background_color;
        Color m_background_light_source_color;

        std::vector<const Shape<N, T>*> m_shape_pointers;
        std::vector<const LightSource<N, T>*> m_light_source_pointers;

        T m_size;

        std::vector<int> m_shape_indices;

        T size() const override
        {
                return m_size;
        }

        std::optional<Intersection<N, T>> intersect(const Ray<N, T>& ray) const override
        {
                return scene_implementation::ray_intersect(m_shape_pointers, m_shape_indices, ray);
        }

        bool has_intersection(const Ray<N, T>& ray, const T& distance) const override
        {
                std::optional<Intersection<N, T>> intersection = intersect(ray);
                return intersection && intersection->distance <= distance;
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

                const BoundingBox<N, T> bounding_box = scene_implementation::compute_bounding_box(m_shape_pointers);

                m_size = (bounding_box.max - bounding_box.min).norm();

                m_shape_indices.resize(m_shape_pointers.size());
                std::iota(m_shape_indices.begin(), m_shape_indices.end(), 0);
        }
};
}
