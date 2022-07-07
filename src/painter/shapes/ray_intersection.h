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
#include <src/numerical/ray.h>

#include <algorithm>
#include <type_traits>

namespace ns::painter
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

template <typename Object, std::size_t N, typename T, typename Shapes, typename Indices>
std::tuple<T, const Object*> ray_intersection(
        const Shapes& shapes,
        const Indices& indices,
        const Ray<N, T>& ray,
        const T max_distance)
{
        using Shape = std::remove_reference_t<decltype(to_ref(shapes.front()))>;

        struct Intersection final
        {
                T distance;
                const Shape* shape;

                Intersection(const T& distance, const Shape* const shape)
                        : distance(distance),
                          shape(shape)
                {
                }

                bool operator<(const Intersection& v) const
                {
                        return distance < v.distance;
                }
        };

        thread_local std::vector<Intersection> intersections;
        intersections.clear();
        intersections.reserve(indices.size());
        for (const auto index : indices)
        {
                const auto& shape = to_ref(shapes[index]);
                const auto distance = shape.intersect_bounds(ray, max_distance);
                if (distance)
                {
                        ASSERT(*distance < max_distance);
                        intersections.emplace_back(*distance, &shape);
                }
        }
        if (intersections.empty())
        {
                return {0, nullptr};
        }

        std::make_heap(intersections.begin(), intersections.end());

        T min_distance = max_distance;
        const Object* closest_object = nullptr;

        do
        {
                const Intersection& bounding = intersections.front();

                if (min_distance < bounding.distance)
                {
                        break;
                }

                const auto [distance, object] = bounding.shape->intersect(ray, min_distance, bounding.distance);
                if (object)
                {
                        ASSERT(distance < min_distance);
                        min_distance = distance;
                        closest_object = object;
                }

                std::pop_heap(intersections.begin(), intersections.end());
                intersections.pop_back();
        } while (!intersections.empty());

        return {min_distance, closest_object};
}
}

template <std::size_t N, typename T, typename Shapes, typename Indices>
auto ray_intersection(const Shapes& shapes, const Indices& indices, const Ray<N, T>& ray, const T max_distance)
        -> decltype(to_ref(shapes.front()).intersect(ray, T(), T()))
{
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
}
