/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "normals.h"

#include "../objects.h"

#include <src/com/exponent.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::painter
{
template <std::size_t N, typename T, typename Color>
bool surface_before_distance(
        const Vector<N, T>& org,
        const SurfacePoint<N, T, Color>& surface,
        const std::optional<T>& distance)
{
        return surface && (!distance || (org - surface.point()).norm_squared() < square(*distance));
}

template <std::size_t N, typename T, typename Color>
bool occluded(
        const Scene<N, T, Color>& scene,
        const Normals<N, T>& normals,
        const Ray<N, T>& ray,
        const std::optional<T>& distance)
{
        if (!normals.smooth)
        {
                if (dot(ray.dir(), normals.geometric) <= 0)
                {
                        return true;
                }
                const SurfacePoint surface = scene.intersect(normals.geometric, ray);
                return surface_before_distance(ray.org(), surface, distance);
        }

        if (dot(ray.dir(), normals.geometric) >= 0)
        {
                const SurfacePoint surface = scene.intersect(normals.geometric, ray);
                return surface_before_distance(ray.org(), surface, distance);
        }

        const SurfacePoint surface_1 = scene.intersect(normals.geometric, ray);
        if (!surface_before_distance(ray.org(), surface_1, distance))
        {
                return true;
        }

        const SurfacePoint surface_2 =
                scene.intersect(surface_1.geometric_normal(), Ray<N, T>(ray).set_org(surface_1.point()));
        return surface_before_distance(ray.org(), surface_2, distance);
}

template <std::size_t N, typename T, typename Color>
SurfacePoint<N, T, Color> intersect(
        const Scene<N, T, Color>& scene,
        const bool smooth_normals,
        const std::optional<Vector<N, T>>& geometric_normal,
        const Ray<N, T>& ray,
        Normals<N, T>* const normals)
{
        SurfacePoint surface = scene.intersect(geometric_normal, ray);
        if (!surface)
        {
                return {};
        }

        {
                Normals<N, T> n = compute_normals(smooth_normals, surface, ray.dir());
                if (!smooth_normals || dot(ray.dir(), n.shading) <= 0)
                {
                        *normals = std::move(n);
                        return surface;
                }
        }

        for (int i = 0; i < 2; ++i)
        {
                surface = scene.intersect(surface.geometric_normal(), Ray<N, T>(ray).set_org(surface.point()));
                if (!surface)
                {
                        return {};
                }
        }

        *normals = compute_normals(smooth_normals, surface, ray.dir());
        return surface;
}
}
