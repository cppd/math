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
namespace visibility_implementation
{
template <std::size_t N, typename T, typename Color>
bool intersection_before(
        const Surface<N, T, Color>* surface,
        const Vector<N, T>& point,
        const std::optional<T>& distance)
{
        return surface && (!distance || (surface->point() - point).norm_squared() < square(*distance));
}
}

template <std::size_t N, typename T, typename Color>
bool occluded(
        const Scene<N, T, Color>& scene,
        const Vector<N, T>& geometric_normal,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        const std::optional<T>& distance)
{
        namespace impl = visibility_implementation;

        const Vector<N, T> point = ray.org();

        const Surface<N, T, Color>* surface;

        if (!smooth_normals || dot(ray.dir(), geometric_normal) >= 0)
        {
                surface = scene.intersect(ray);
                return impl::intersection_before(surface, point, distance);
        }

        surface = scene.intersect(ray);
        if (!impl::intersection_before(surface, point, distance))
        {
                return true;
        }

        surface = scene.intersect(Ray<N, T>(ray).set_org(surface->point()));
        return impl::intersection_before(surface, point, distance);
}
}
