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
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <optional>

namespace ns::painter
{
template <std::size_t N, typename T, typename SurfacePoint>
bool surface_before_distance(const Vector<N, T>& org, const SurfacePoint& surface, const std::optional<T>& distance)
{
        return surface && (!distance || (org - surface.point()).norm_squared() < square(*distance));
}

template <std::size_t N, typename T, typename Color>
bool occluded(
        const Scene<N, T, Color>& scene,
        const Vector<N, T>& geometric_normal,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        const std::optional<T>& distance)
{
        if (!smooth_normals)
        {
                if (dot(ray.dir(), geometric_normal) <= 0)
                {
                        return true;
                }
                const auto surface = scene.intersect(geometric_normal, ray);
                return surface_before_distance(ray.org(), surface, distance);
        }

        if (dot(ray.dir(), geometric_normal) >= 0)
        {
                const auto surface = scene.intersect(geometric_normal, ray);
                return surface_before_distance(ray.org(), surface, distance);
        }

        const auto surface_1 = scene.intersect(geometric_normal, ray);
        if (!surface_before_distance(ray.org(), surface_1, distance))
        {
                return true;
        }

        const auto surface_2 = scene.intersect(surface_1.geometric_normal(), Ray<N, T>(ray).set_org(surface_1.point()));
        return surface_before_distance(ray.org(), surface_2, distance);
}
}
