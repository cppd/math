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

#include "../shapes/shape.h"

#include <src/com/type/limit.h>
#include <src/numerical/ray.h>

#include <algorithm>
#include <tuple>
#include <vector>

namespace painter::scenes_implementation
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
std::optional<Intersection<N, T>> ray_intersect(const std::vector<const Shape<N, T>*>& shapes, const Ray<N, T>& ray)
{
        if (shapes.size() == 1)
        {
                std::optional<T> bounding_distance = shapes[0]->intersect_bounding(ray);
                if (bounding_distance)
                {
                        return shapes[0]->intersect(ray, *bounding_distance);
                }
                return std::nullopt;
        }

        // Объекты могут быть сложными, поэтому перед поиском точного пересечения
        // их надо разместить по возрастанию примерного пересечения.

        std::vector<std::tuple<T, const Shape<N, T>*>> intersections;
        intersections.reserve(shapes.size());

        for (const Shape<N, T>* shape : shapes)
        {
                std::optional<T> distance = shape->intersect_bounding(ray);
                if (distance)
                {
                        intersections.emplace_back(*distance, shape);
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
bool ray_has_intersection(const std::vector<const Shape<N, T>*>& shapes, const Ray<N, T>& ray, const T& distance)
{
        for (const Shape<N, T>* shape : shapes)
        {
                std::optional<T> bounding_distance = shape->intersect_bounding(ray);
                if (!bounding_distance || *bounding_distance >= distance)
                {
                        continue;
                }

                std::optional<Intersection<N, T>> intersection = shape->intersect(ray, *bounding_distance);
                if (!intersection || intersection->distance >= distance)
                {
                        continue;
                }

                return true;
        }

        return false;
}
}
