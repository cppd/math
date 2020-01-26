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

#include "com/ray.h"
#include "com/type/limit.h"

#include <algorithm>
#include <tuple>

template <size_t N, typename T, typename Object, typename Surface, typename Data>
bool ray_intersection(const std::vector<const Object*>& objects, const Ray<N, T>& ray, T* intersection_distance,
                      const Surface** intersection_surface, const Data** intersection_data)
{
        if (objects.size() == 1)
        {
                T approximate_distance;
                T distance;
                const Surface* surface;
                const Data* data;

                if (objects[0]->intersect_approximate(ray, &approximate_distance) &&
                    objects[0]->intersect_precise(ray, approximate_distance, &distance, &surface, &data))
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

        std::vector<std::tuple<T, const Object*>> approximate_intersections;
        approximate_intersections.reserve(objects.size());

        for (const Object* obj : objects)
        {
                T distance;
                if (obj->intersect_approximate(ray, &distance))
                {
                        approximate_intersections.emplace_back(distance, obj);
                }
        }

        if (approximate_intersections.empty())
        {
                return false;
        }

        std::sort(approximate_intersections.begin(), approximate_intersections.end(),
                  [](const std::tuple<T, const Object*>& a, const std::tuple<T, const Object*>& b) {
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
                const Surface* surface;
                const Data* data;

                if (object->intersect_precise(ray, approximate_distance, &distance, &surface, &data) && distance < min_distance)
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

template <size_t N, typename T, typename Object>
bool ray_intersection(const std::vector<const Object*>& objects, const Ray<N, T>& ray, T* intersection_distance,
                      const Object** intersection_object)
{
        T min_distance = limits<T>::max();
        bool found = false;

        for (const Object* obj : objects)
        {
                T distance;
                if (obj->intersect(ray, &distance) && distance < min_distance)
                {
                        min_distance = distance;
                        *intersection_object = obj;
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

template <size_t N, typename T, typename Object>
bool ray_intersection(const std::vector<Object>& objects, const std::vector<int>& object_indices, const Ray<N, T>& ray,
                      T* intersection_distance, const Object** intersection_object)
{
        T min_distance = limits<T>::max();
        bool found = false;

        for (int object_index : object_indices)
        {
                T distance;
                if (objects[object_index].intersect(ray, &distance) && distance < min_distance)
                {
                        min_distance = distance;
                        *intersection_object = &objects[object_index];
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
