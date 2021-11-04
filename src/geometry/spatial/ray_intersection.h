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

#include <src/com/type/limit.h>
#include <src/numerical/ray.h>

#include <optional>
#include <tuple>

namespace ns::geometry
{
template <std::size_t N, typename T, typename Object, typename Indices>
std::optional<std::tuple<T, const Object*>> ray_intersection(
        const std::vector<Object>& objects,
        const Indices& indices,
        const Ray<N, T>& ray)
{
        T min_distance = Limits<T>::max();
        const Object* closest_object = nullptr;

        for (const auto index : indices)
        {
                const std::optional<T> distance = objects[index].intersect(ray);
                if (distance && *distance < min_distance)
                {
                        min_distance = *distance;
                        closest_object = &objects[index];
                }
        }

        if (closest_object)
        {
                return std::make_tuple(min_distance, closest_object);
        }
        return std::nullopt;
}
}
