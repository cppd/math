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
#include <tuple>

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
bool light_source_occluded(
        const Scene<N, T, Color>& scene,
        const Normals<N, T>& normals,
        const Ray<N, T>& ray,
        const std::optional<T>& distance)
{
        ASSERT(dot(ray.dir(), normals.shading) > 0);

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

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
std::tuple<SurfacePoint<N, T, Color>, Normals<N, T>> scene_intersect(
        const Scene<N, T, Color>& scene,
        const std::optional<Vector<N, T>>& geometric_normal,
        const Ray<N, T>& ray)
{
        SurfacePoint<N, T, Color> surface = scene.intersect(geometric_normal, ray);
        if (!surface)
        {
                return {};
        }

        Normals<N, T> normals = compute_normals<FLAT_SHADING>(surface, ray.dir());
        if (FLAT_SHADING || dot(ray.dir(), normals.shading) <= 0)
        {
                return {surface, normals};
        }

        for (int i = 0; i < 2; ++i)
        {
                surface = scene.intersect(surface.geometric_normal(), Ray<N, T>(ray).set_org(surface.point()));
                if (!surface)
                {
                        return {};
                }
        }

        return {surface, compute_normals<FLAT_SHADING>(surface, ray.dir())};
}
}
