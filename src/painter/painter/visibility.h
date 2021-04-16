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

#include <src/com/math.h>

namespace ns::painter
{
namespace visibility_implementation
{
template <std::size_t N, typename T>
bool intersection_before(
        const std::optional<Intersection<N, T>>& intersection,
        const Vector<N, T>& point,
        const std::optional<T>& distance)
{
        return intersection && (!distance || (intersection->point - point).norm_squared() < square(*distance));
}
}

template <std::size_t N, typename T>
bool occluded(
        const Scene<N, T>& scene,
        const Vector<N, T>& geometric_normal,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        const std::optional<T>& distance)
{
        namespace impl = visibility_implementation;

        const Vector<N, T> point = ray.org();

        std::optional<Intersection<N, T>> intersection;

        if (!smooth_normals || dot(ray.dir(), geometric_normal) >= 0)
        {
                // Если объект не состоит из симплексов или геометрическая сторона обращена
                // к источнику света, то напрямую рассчитать видимость источника света.
                intersection = scene.intersect(ray);
                return impl::intersection_before(intersection, point, distance);
        }

        // Если объект состоит из симплексов и геометрическая сторона направлена
        // от источника  света, то геометрически она не освещена, но из-за нормалей
        // у вершин, дающих сглаживание, она может быть «освещена», и надо определить,
        // находится ли она в тени без учёта тени от самой поверхности в окрестности
        // точки. Это можно сделать направлением луча к источнику света с игнорированием
        // самого первого пересечения в предположении, что оно произошло с этой самой
        // окрестностью точки.

        intersection = scene.intersect(ray);
        if (!impl::intersection_before(intersection, point, distance))
        {
                // Если луч к источнику света направлен внутрь поверхности, и нет повторного
                // пересечения с поверхностью до источника света, то нет освещения в точке.
                return true;
        }

        intersection = scene.intersect(Ray<N, T>(ray).set_org(intersection->point));
        return impl::intersection_before(intersection, point, distance);
}
}
