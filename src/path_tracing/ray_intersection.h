/*
Copyright (C) 2017 Topological Manifold

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

#include "objects.h"

#include "com/ray.h"

#include <algorithm>
#include <limits>
#include <type_traits>

namespace RayIntersectionImplementation
{
template <typename Object>
class FirstIntersection
{
        static_assert(std::is_base_of_v<GenericObject, Object>);

        double m_t;
        const Object* m_object;

public:
        FirstIntersection(double t, const Object* object) : m_t(t), m_object(object)
        {
        }
        double get_t() const
        {
                return m_t;
        }
        const Object* get_object() const
        {
                return m_object;
        }
        bool operator<(const FirstIntersection& p) const
        {
                return m_t < p.m_t;
        }
};
}

template <typename Object>
std::enable_if_t<std::is_base_of_v<GenericObject, Object>, bool> ray_intersection(const std::vector<const Object*>& objects,
                                                                                  const ray3& ray, double* t,
                                                                                  const Surface** found_surface,
                                                                                  const GeometricObject** found_object)
{
        static_assert(std::is_base_of_v<GenericObject, Object>);

        using namespace RayIntersectionImplementation;

        // Объекты могут быть сложными, поэтому перед поиском точного пересечения
        // их надо разместить по возрастанию примерного пересечения.

        std::vector<FirstIntersection<Object>> first_intersection;
        first_intersection.reserve(objects.size());

        for (const Object* obj : objects)
        {
                double distance;
                if (obj->intersect_approximate(ray, &distance))
                {
                        first_intersection.emplace_back(distance, obj);
                }
        }

        std::sort(first_intersection.begin(), first_intersection.end());

        double min_distance = std::numeric_limits<double>::max();
        bool found = false;

        for (const FirstIntersection<Object>& fi : first_intersection)
        {
                if (min_distance < fi.get_t())
                {
                        break;
                }

                double distance;
                const Surface* surface;
                const GeometricObject* object;

                if (fi.get_object()->intersect_precise(ray, fi.get_t(), &distance, &surface, &object) && distance < min_distance)
                {
                        min_distance = distance;
                        *found_surface = surface;
                        *found_object = object;
                        found = true;
                }
        }

        if (found)
        {
                *t = min_distance;
                return true;
        }

        return false;
}

template <typename Object>
std::enable_if_t<std::is_base_of_v<GeometricObject, Object>, bool> ray_intersection(const std::vector<const Object*>& objects,
                                                                                    const ray3& ray, double* t,
                                                                                    const Object** found_object)
{
        double min_distance = std::numeric_limits<double>::max();
        bool found = false;

        for (const Object* obj : objects)
        {
                double distance;
                if (obj->intersect(ray, &distance) && distance < min_distance)
                {
                        min_distance = distance;
                        *found_object = obj;
                        found = true;
                }
        }

        if (found)
        {
                *t = min_distance;
                return true;
        }

        return false;
}

template <typename Object>
std::enable_if_t<std::is_base_of_v<GeometricObject, Object>, bool> ray_intersection(const std::vector<Object>& objects,
                                                                                    const std::vector<int>& object_indices,
                                                                                    const ray3& ray, double* t,
                                                                                    const Object** found_object)
{
        double min_distance = std::numeric_limits<double>::max();
        bool found = false;

        for (int object_index : object_indices)
        {
                double distance;
                if (objects[object_index].intersect(ray, &distance) && distance < min_distance)
                {
                        min_distance = distance;
                        *found_object = &objects[object_index];
                        found = true;
                }
        }

        if (found)
        {
                *t = min_distance;
                return true;
        }

        return false;
}
