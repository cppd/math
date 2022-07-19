/*
Copyright (C) 2017-2022 Topological Manifold

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
#include <src/com/reference.h>
#include <src/com/type/limit.h>
#include <src/numerical/ray.h>

#include <algorithm>
#include <type_traits>

namespace ns::painter::scenes
{
namespace ray_intersection_implementation
{
template <typename Object, typename Shape, std::size_t N, typename T>
std::tuple<T, const Object*> ray_intersection(const Shape& shape, const Ray<N, T>& ray, const T max_distance)
{
        const auto distance = shape.intersect_bounds(ray, max_distance);
        if (distance)
        {
                return shape.intersect(ray, max_distance, *distance);
        }
        return {0, nullptr};
}

template <typename T, typename Shape>
struct BoundingIntersection final
{
        T distance;
        const Shape* shape;

        BoundingIntersection(const T distance, const Shape* const shape)
                : distance(distance),
                  shape(shape)
        {
        }

        [[nodiscard]] bool operator<(const BoundingIntersection& v) const
        {
                return distance < v.distance;
        }
};

template <typename T, typename Shape>
class BoundingIntersectionHeap final
{
        std::vector<BoundingIntersection<T, Shape>> intersections_;

public:
        template <std::size_t N, typename Shapes, typename Indices>
        void make(const Shapes& shapes, const Indices& indices, const Ray<N, T>& ray, const T max_distance)
        {
                intersections_.clear();
                intersections_.reserve(indices.size());

                for (const auto index : indices)
                {
                        const auto& shape = to_ref(shapes[index]);
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

        [[nodiscard]] const BoundingIntersection<T, Shape>& front() const
        {
                return intersections_.front();
        }

        void pop()
        {
                std::pop_heap(intersections_.begin(), intersections_.end());
                intersections_.pop_back();
        }
};

template <typename Object, std::size_t N, typename T, typename Shapes, typename Indices>
std::tuple<T, const Object*> ray_intersection(
        const Shapes& shapes,
        const Indices& indices,
        const Ray<N, T>& ray,
        const T max_distance)
{
        using Shape = std::remove_reference_t<decltype(to_ref(shapes.front()))>;

        thread_local BoundingIntersectionHeap<T, Shape> heap;

        T min_distance = Limits<T>::infinity();
        const Object* closest_object = nullptr;

        for (heap.make(shapes, indices, ray, max_distance); !heap.empty(); heap.pop())
        {
                const BoundingIntersection<T, Shape>& bound = heap.front();

                if (min_distance < bound.distance)
                {
                        break;
                }

                const auto [distance, object] = bound.shape->intersect(ray, min_distance, bound.distance);
                if (object)
                {
                        ASSERT(distance < min_distance);
                        min_distance = distance;
                        closest_object = object;
                }
        }

        return {min_distance, closest_object};
}

template <typename Shape, std::size_t N, typename T>
bool ray_intersection_any(const Shape& shape, const Ray<N, T>& ray, const T max_distance)
{
        const auto distance = shape.intersect_bounds(ray, max_distance);
        if (distance)
        {
                return shape.intersect_any(ray, max_distance, *distance);
        }
        return false;
}

template <typename Shapes, typename Indices, std::size_t N, typename T>
bool ray_intersection_any(const Shapes& shapes, const Indices& indices, const Ray<N, T>& ray, const T max_distance)
{
        for (const auto index : indices)
        {
                if (ray_intersection_any(to_ref(shapes[index]), ray, max_distance))
                {
                        return true;
                }
        }
        return false;
}
}

template <std::size_t N, typename T, typename Shapes, typename Indices>
[[nodiscard]] auto ray_intersection(
        const Shapes& shapes,
        const Indices& indices,
        const Ray<N, T>& ray,
        const T max_distance) -> decltype(to_ref(shapes.front()).intersect(ray, T(), T()))
{
        static_assert(std::is_floating_point_v<T>);

        namespace impl = ray_intersection_implementation;

        using Tuple = decltype(to_ref(shapes.front()).intersect(ray, T(), T()));
        static_assert(2 == std::tuple_size_v<Tuple>);
        static_assert(std::is_same_v<T, std::tuple_element_t<0, Tuple>>);
        static_assert(std::is_pointer_v<std::tuple_element_t<1, Tuple>>);

        using Object = std::remove_pointer_t<std::tuple_element_t<1, Tuple>>;

        if (indices.size() == 1)
        {
                return impl::ray_intersection<Object>(to_ref(shapes[indices.front()]), ray, max_distance);
        }

        return impl::ray_intersection<Object>(shapes, indices, ray, max_distance);
}

template <std::size_t N, typename T, typename Shapes, typename Indices>
[[nodiscard]] bool ray_intersection_any(
        const Shapes& shapes,
        const Indices& indices,
        const Ray<N, T>& ray,
        const T max_distance)
{
        static_assert(std::is_floating_point_v<T>);

        namespace impl = ray_intersection_implementation;

        if (indices.size() == 1)
        {
                return impl::ray_intersection_any(to_ref(shapes[indices.front()]), ray, max_distance);
        }

        return impl::ray_intersection_any(shapes, indices, ray, max_distance);
}
}
