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
#include <src/com/reference.h>
#include <src/numerical/ray.h>

#include <algorithm>
#include <type_traits>

namespace ns::painter
{
namespace ray_intersection_implementation
{
template <typename Result, typename Shape, std::size_t N, typename T>
Result ray_intersection(const Shape& shape, const Ray<N, T>& ray, const T max_distance)
{
        const auto distance = shape.intersect_bounding(ray, max_distance);
        if (distance)
        {
                return shape.intersect(ray, max_distance, *distance);
        }
        return Result(nullptr);
}

template <typename Result, std::size_t N, typename T, typename Shapes, typename Indices>
Result ray_intersection(const Shapes& shapes, const Indices& indices, const Ray<N, T>& ray, const T max_distance)
{
        using Shape = std::remove_reference_t<decltype(to_ref(shapes.front()))>;

        struct Intersection final
        {
                T distance;
                const Shape* shape;
                Intersection(const T& distance, const Shape* const shape) : distance(distance), shape(shape)
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
                const auto distance = shape.intersect_bounding(ray, max_distance);
                if (distance)
                {
                        ASSERT(*distance < max_distance);
                        intersections.emplace_back(*distance, &shape);
                }
        }
        if (intersections.empty())
        {
                return Result(nullptr);
        }

        std::make_heap(intersections.begin(), intersections.end());

        Result min;
        min.distance = max_distance;
        min.surface = nullptr;

        do
        {
                const Intersection& bounding = intersections.front();

                if (min.distance < bounding.distance)
                {
                        break;
                }

                const Result intersection = bounding.shape->intersect(ray, min.distance, bounding.distance);
                if (intersection.surface)
                {
                        ASSERT(intersection.distance < min.distance);
                        min = intersection;
                }

                std::pop_heap(intersections.begin(), intersections.end());
                intersections.pop_back();
        } while (!intersections.empty());

        return min;
}
}

template <std::size_t N, typename T, typename Shapes, typename Indices>
auto ray_intersection(const Shapes& shapes, const Indices& indices, const Ray<N, T>& ray, const T max_distance)
        -> decltype(to_ref(shapes.front()).intersect(ray, T(), T()))
{
        namespace impl = ray_intersection_implementation;

        using Result = decltype(to_ref(shapes.front()).intersect(ray, T(), T()));

        if (indices.size() == 1)
        {
                return impl::ray_intersection<Result>(to_ref(shapes[indices.front()]), ray, max_distance);
        }

        return impl::ray_intersection<Result>(shapes, indices, ray, max_distance);
}
}
