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

#include "normals.h"

#include <src/com/type/limit.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>

#include <cstddef>
#include <optional>
#include <tuple>

namespace ns::painter::integrators
{
namespace visibility_implementation
{
template <typename T>
[[nodiscard]] T visibility_distance(const T distance)
{
        static constexpr T EPSILON = 1000 * Limits<T>::epsilon();
        static_assert(EPSILON > 0 && EPSILON < 1);
        static constexpr T K = 1 - EPSILON;
        return K * distance;
}

template <typename T>
[[nodiscard]] bool directed_outside(const T cosine)
{
        static constexpr T EPSILON = 100 * Limits<T>::epsilon();
        return cosine > EPSILON;
}
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] bool occluded(
        const Scene<N, T, Color>& scene,
        const Normals<N, T>& normals,
        const Ray<N, T>& ray,
        const std::optional<T>& distance)
{
        namespace impl = visibility_implementation;

        if (!impl::directed_outside(dot(ray.dir(), normals.shading)))
        {
                return true;
        }

        const T d = distance ? impl::visibility_distance(*distance) : Limits<T>::infinity();

        if (dot(ray.dir(), normals.geometric) >= 0)
        {
                return scene.intersect_any(normals.geometric, ray, d);
        }

        const SurfaceIntersection surface = scene.intersect(normals.geometric, ray, d);
        if (!surface)
        {
                return false;
        }

        const T d_2 = d - surface.distance();
        if (d_2 > 0)
        {
                return scene.intersect_any(surface.geometric_normal(), Ray<N, T>(ray).set_org(surface.point()), d_2);
        }
        return false;
}

template <std::size_t N, typename T, typename Color>
[[nodiscard]] bool occluded(
        const Scene<N, T, Color>& scene,
        const Vector<N, T>& point_1,
        const Normals<N, T>& normals_1,
        const Vector<N, T>& point_2,
        const Normals<N, T>& normals_2)
{
        namespace impl = visibility_implementation;

        const Vector<N, T> direction_1 = point_2 - point_1;
        const Ray<N, T> ray_1(point_1, direction_1);

        if (!impl::directed_outside(dot(ray_1.dir(), normals_1.shading)))
        {
                return true;
        }

        if (!impl::directed_outside(-dot(ray_1.dir(), normals_2.shading)))
        {
                return true;
        }

        const bool visible_1 = dot(ray_1.dir(), normals_1.geometric) >= 0;
        const bool visible_2 = dot(ray_1.dir(), normals_2.geometric) <= 0;

        T distance = impl::visibility_distance(direction_1.norm());

        if (visible_1 && visible_2)
        {
                return scene.intersect_any(normals_1.geometric, ray_1, distance);
        }

        if (!visible_1)
        {
                const SurfaceIntersection surface = scene.intersect(normals_1.geometric, ray_1, distance);
                if (!surface)
                {
                        return false;
                }

                distance -= surface.distance();
                if (distance <= 0)
                {
                        return false;
                }
        }

        const Ray<N, T> ray_2 = ray_1.reversed().set_org(point_2);

        if (!visible_2)
        {
                const SurfaceIntersection surface = scene.intersect(normals_2.geometric, ray_2, distance);
                if (!surface)
                {
                        return false;
                }

                distance -= surface.distance();
                if (distance <= 0)
                {
                        return false;
                }

                return scene.intersect_any(
                        surface.geometric_normal(), Ray<N, T>(ray_2).set_org(surface.point()), distance);
        }

        return scene.intersect_any(normals_2.geometric, ray_2, distance);
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
[[nodiscard]] std::tuple<SurfaceIntersection<N, T, Color>, Normals<N, T>> scene_intersect(
        const Scene<N, T, Color>& scene,
        const std::optional<Vector<N, T>>& geometric_normal,
        const Ray<N, T>& ray)
{
        SurfaceIntersection<N, T, Color> surface = scene.intersect(geometric_normal, ray);
        if (!surface)
        {
                return {};
        }

        {
                const Normals<N, T> normals = compute_normals<FLAT_SHADING>(surface, ray.dir());
                if (FLAT_SHADING || dot(ray.dir(), normals.shading) <= 0)
                {
                        return {surface, normals};
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

        return {surface, compute_normals<FLAT_SHADING>(surface, ray.dir())};
}
}
