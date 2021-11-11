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

#include "../objects.h"

#include <src/com/exponent.h>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
bool surface_before_distance(
        const Vector<N, T>& point,
        const Surface<N, T, Color>* const surface,
        const std::optional<T>& distance)
{
        return surface && (!distance || (surface->point() - point).norm_squared() < square(*distance));
}

template <std::size_t N, typename T, typename Color>
bool occluded(
        const Scene<N, T, Color>& scene,
        const Vector<N, T>& geometric_normal,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        const std::optional<T>& distance)
{
        const Vector<N, T> point = ray.org();

        if (!smooth_normals || dot(ray.dir(), geometric_normal) >= 0)
        {
                const auto [_, surface] = scene.intersect(ray);
                return surface_before_distance(point, surface, distance);
        }

        const auto [distance_1, surface_1] = scene.intersect(ray);
        if (!surface_before_distance(point, surface_1, distance))
        {
                return true;
        }

        const auto [distance_2, surface_2] = scene.intersect(Ray<N, T>(ray).set_org(surface_1->point()));
        return surface_before_distance(point, surface_2, distance);
}
}
