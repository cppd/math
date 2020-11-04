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

#include "shape.h"

#include <src/com/type/limit.h>
#include <src/numerical/ray.h>

#include <algorithm>
#include <tuple>
#include <vector>

namespace painter
{
template <size_t N, typename T>
T scene_size(const std::vector<const Shape<N, T>*>& shapes)
{
        BoundingBox<N, T> bb = shapes[0]->bounding_box();
        for (size_t i = 1; i < shapes.size(); ++i)
        {
                BoundingBox<N, T> shape_bb = shapes[i]->bounding_box();
                bb.min = min_vector(bb.min, shape_bb.min);
                bb.max = max_vector(bb.max, shape_bb.max);
        }
        return (bb.max - bb.min).norm();
}

template <size_t N, typename T>
bool ray_intersect(
        const std::vector<const Shape<N, T>*>& shapes,
        const Ray<N, T>& ray,
        T* intersection_distance,
        const Surface<N, T>** intersection_surface,
        const void** intersection_data)
{
        if (shapes.size() == 1)
        {
                T approximate_distance;
                T distance;
                const Surface<N, T>* surface;
                const void* data;

                if (shapes[0]->intersect_approximate(ray, &approximate_distance)
                    && shapes[0]->intersect_precise(ray, approximate_distance, &distance, &surface, &data))
                {
                        *intersection_distance = distance;
                        *intersection_surface = surface;
                        *intersection_data = data;

                        return true;
                }

                return false;
        }

        // Объекты могут быть сложными, поэтому перед поиском точного пересечения
        // их надо разместить по возрастанию примерного пересечения.

        std::vector<std::tuple<T, const Shape<N, T>*>> approximate_intersections;
        approximate_intersections.reserve(shapes.size());

        for (const Shape<N, T>* shape : shapes)
        {
                T distance;
                if (shape->intersect_approximate(ray, &distance))
                {
                        approximate_intersections.emplace_back(distance, shape);
                }
        }

        if (approximate_intersections.empty())
        {
                return false;
        }

        std::sort(
                approximate_intersections.begin(), approximate_intersections.end(),
                [](const std::tuple<T, const Shape<N, T>*>& a, const std::tuple<T, const Shape<N, T>*>& b) {
                        return std::get<0>(a) < std::get<0>(b);
                });

        T min_distance = limits<T>::max();
        bool found = false;

        for (const auto& [approximate_distance, object] : approximate_intersections)
        {
                if (min_distance < approximate_distance)
                {
                        break;
                }

                T distance;
                const Surface<N, T>* surface;
                const void* data;

                if (object->intersect_precise(ray, approximate_distance, &distance, &surface, &data)
                    && distance < min_distance)
                {
                        min_distance = distance;
                        *intersection_surface = surface;
                        *intersection_data = data;
                        found = true;
                }
        }

        if (found)
        {
                *intersection_distance = min_distance;
                return true;
        }

        return false;
}

template <size_t N, typename T>
bool ray_has_intersection(const std::vector<const Shape<N, T>*>& shapes, const Ray<N, T>& ray, const T& distance)
{
        for (const Shape<N, T>* shape : shapes)
        {
                T distance_to_object;
                const Surface<N, T>* surface;
                const void* intersection_data;

                if (!shape->intersect_approximate(ray, &distance_to_object))
                {
                        continue;
                }

                if (distance_to_object >= distance)
                {
                        continue;
                }

                if (!shape->intersect_precise(
                            ray, distance_to_object, &distance_to_object, &surface, &intersection_data))
                {
                        continue;
                }

                if (distance_to_object >= distance)
                {
                        continue;
                }

                return true;
        }

        return false;
}
}
