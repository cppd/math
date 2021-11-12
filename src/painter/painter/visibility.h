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

#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <optional>

namespace ns::painter
{
template <typename T, typename Surface>
bool surface_before_distance(const T& distance, const Surface* const surface, const std::optional<T>& max_distance)
{
        return surface && (!max_distance || distance < *max_distance);
}

template <std::size_t N, typename T, typename Scene>
bool occluded(
        const Scene& scene,
        const Vector<N, T>& geometric_normal,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        const std::optional<T>& max_distance)
{
        if (!smooth_normals || dot(ray.dir(), geometric_normal) >= 0)
        {
                const auto [distance, surface] = scene.intersect(ray);
                return surface_before_distance(distance, surface, max_distance);
        }

        const auto [distance_1, surface_1] = scene.intersect(ray);
        if (!surface_before_distance(distance_1, surface_1, max_distance))
        {
                return true;
        }

        const Vector<N, T> point = surface_1->point(ray, distance_1);
        const auto [distance_2, surface_2] = scene.intersect(Ray<N, T>(ray).set_org(point));
        return surface_before_distance(distance_2, surface_2, max_distance);
}
}
