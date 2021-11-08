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

#include "shape.h"

#include <src/com/error.h>
#include <src/numerical/ray.h>

#include <algorithm>
#include <vector>

namespace ns::painter
{
namespace ray_intersection_implementation
{
template <std::size_t N, typename T, typename Color>
struct BoundingIntersection final
{
        T distance;
        const Shape<N, T, Color>* shape;

        BoundingIntersection(const T& distance, const Shape<N, T, Color>* const shape)
                : distance(distance), shape(shape)
        {
        }

        bool operator<(const BoundingIntersection& v) const
        {
                return distance < v.distance;
        }
};
}

template <std::size_t N, typename T, typename Color, typename Indices>
ShapeIntersection<N, T, Color> ray_intersection(
        const std::vector<const Shape<N, T, Color>*>& shapes,
        const Indices& indices,
        const Ray<N, T>& ray,
        const T max_distance)
{
        namespace impl = ray_intersection_implementation;

        if (indices.size() == 1)
        {
                const std::optional<T> distance = shapes[indices.front()]->intersect_bounding(ray, max_distance);
                if (distance)
                {
                        return shapes[indices.front()]->intersect(ray, max_distance, *distance);
                }
                return ShapeIntersection<N, T, Color>(nullptr);
        }

        thread_local std::vector<impl::BoundingIntersection<N, T, Color>> intersections;
        intersections.clear();
        intersections.reserve(indices.size());
        for (const auto index : indices)
        {
                const std::optional<T> distance = shapes[index]->intersect_bounding(ray, max_distance);
                if (distance)
                {
                        ASSERT(*distance < max_distance);
                        intersections.emplace_back(*distance, shapes[index]);
                }
        }
        if (intersections.empty())
        {
                return ShapeIntersection<N, T, Color>(nullptr);
        }

        std::make_heap(intersections.begin(), intersections.end());

        T min_distance = max_distance;
        ShapeIntersection<N, T, Color> closest_intersection(nullptr);

        do
        {
                const impl::BoundingIntersection<N, T, Color>& bounding = intersections.front();

                if (min_distance < bounding.distance)
                {
                        break;
                }

                const ShapeIntersection<N, T, Color> intersection =
                        bounding.shape->intersect(ray, min_distance, bounding.distance);
                if (intersection.surface)
                {
                        ASSERT(intersection.distance < min_distance);
                        min_distance = intersection.distance;
                        closest_intersection = intersection;
                }

                std::pop_heap(intersections.begin(), intersections.end());
                intersections.pop_back();
        } while (!intersections.empty());

        return closest_intersection;
}

}
