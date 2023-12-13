/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/numerical/ray.h>

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <vector>

namespace ns::painter::scenes
{
namespace ray_intersection_implementation
{
template <std::size_t N, typename T, typename Color>
struct BoundingIntersection final
{
        T distance;
        const Shape<N, T, Color>* shape;

        BoundingIntersection(const T distance, const Shape<N, T, Color>* const shape)
                : distance(distance),
                  shape(shape)
        {
        }

        [[nodiscard]] bool operator<(const BoundingIntersection& v) const
        {
                return distance < v.distance;
        }
};

template <std::size_t N, typename T, typename Color>
class BoundingIntersectionHeap final
{
        std::vector<BoundingIntersection<N, T, Color>> intersections_;

public:
        template <typename Indices>
        void make(
                const std::vector<const Shape<N, T, Color>*>& shapes,
                const Indices& indices,
                const Ray<N, T>& ray,
                const T max_distance)
        {
                intersections_.clear();
                intersections_.reserve(indices.size());

                for (const auto index : indices)
                {
                        const Shape<N, T, Color>& shape = *shapes[index];
                        const auto distance = shape.intersect_bounds(ray, max_distance);
                        if (distance)
                        {
                                ASSERT(*distance < max_distance);
                                intersections_.emplace_back(*distance, &shape);
                        }
                }

                std::make_heap(intersections_.begin(), intersections_.end());
        }

        [[nodiscard]] bool empty() const
        {
                return intersections_.empty();
        }

        [[nodiscard]] const BoundingIntersection<N, T, Color>& front() const
        {
                return intersections_.front();
        }

        void pop()
        {
                std::pop_heap(intersections_.begin(), intersections_.end());
                intersections_.pop_back();
        }
};

template <std::size_t N, typename T, typename Color, typename Indices>
ShapeIntersection<N, T, Color> ray_intersection(
        const std::vector<const Shape<N, T, Color>*>& shapes,
        const Indices& indices,
        const Ray<N, T>& ray,
        const T max_distance)
{
        thread_local BoundingIntersectionHeap<N, T, Color> heap;

        ShapeIntersection<N, T, Color> res{max_distance, nullptr};

        for (heap.make(shapes, indices, ray, max_distance); !heap.empty(); heap.pop())
        {
                const BoundingIntersection<N, T, Color>& bounding = heap.front();

                if (res.distance < bounding.distance)
                {
                        break;
                }

                const ShapeIntersection<N, T, Color> intersection =
                        bounding.shape->intersect(ray, res.distance, bounding.distance);

                if (intersection.surface)
                {
                        ASSERT(intersection.distance < res.distance);
                        res = intersection;
                }
        }

        return res;
}
}

template <std::size_t N, typename T, typename Color, typename Indices>
[[nodiscard]] ShapeIntersection<N, T, Color> ray_intersection(
        const std::vector<const Shape<N, T, Color>*>& shapes,
        const Indices& indices,
        const Ray<N, T>& ray,
        const T max_distance)
{
        static_assert(std::is_floating_point_v<T>);

        if (indices.size() == 1)
        {
                const Shape<N, T, Color>& shape = *shapes[indices.front()];
                const auto distance = shape.intersect_bounds(ray, max_distance);
                if (distance)
                {
                        return shape.intersect(ray, max_distance, *distance);
                }
                return {Limits<T>::infinity(), nullptr};
        }

        return ray_intersection_implementation::ray_intersection(shapes, indices, ray, max_distance);
}

template <std::size_t N, typename T, typename Color, typename Indices>
[[nodiscard]] bool ray_intersection_any(
        const std::vector<const Shape<N, T, Color>*>& shapes,
        const Indices& indices,
        const Ray<N, T>& ray,
        const T max_distance)
{
        static_assert(std::is_floating_point_v<T>);

        const auto any_intersection = [&ray, max_distance](const Shape<N, T, Color>& shape)
        {
                const auto distance = shape.intersect_bounds(ray, max_distance);
                if (distance)
                {
                        return shape.intersect_any(ray, max_distance, *distance);
                }
                return false;
        };

        if (indices.size() == 1)
        {
                return any_intersection(*shapes[indices.front()]);
        }

        for (const auto index : indices)
        {
                if (any_intersection(*shapes[index]))
                {
                        return true;
                }
        }
        return false;
}
}
