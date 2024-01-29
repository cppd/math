/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/reference.h>
#include <src/numerical/ray.h>

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace ns::geometry::spatial
{
template <std::size_t N, typename T, typename Objects, typename Indices>
[[nodiscard]] auto ray_intersection(
        const Objects& objects,
        const Indices& indices,
        const numerical::Ray<N, T>& ray,
        const T& max_distance) -> std::tuple<T, const std::remove_reference_t<decltype(to_ref(objects.front()))>*>
{
        using Object = std::remove_reference_t<decltype(to_ref(objects.front()))>;

        T min_distance = max_distance;
        const Object* closest_object = nullptr;

        for (const auto index : indices)
        {
                const auto distance = to_ref(objects[index]).intersect(ray);
                static_assert(std::is_same_v<T, std::remove_cvref_t<decltype(*distance)>>);
                if (distance && *distance < min_distance)
                {
                        min_distance = *distance;
                        closest_object = &to_ref(objects[index]);
                }
        }

        return {min_distance, closest_object};
}

template <std::size_t N, typename T, typename Objects, typename Indices>
[[nodiscard]] bool ray_intersection_any(
        const Objects& objects,
        const Indices& indices,
        const numerical::Ray<N, T>& ray,
        const T& max_distance)
{
        for (const auto index : indices)
        {
                const auto distance = to_ref(objects[index]).intersect(ray);
                static_assert(std::is_same_v<T, std::remove_cvref_t<decltype(*distance)>>);
                if (distance && *distance < max_distance)
                {
                        return true;
                }
        }
        return false;
}
}
