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

#include "trace.h"

#include <src/com/error.h>
#include <src/com/math.h>

#include <algorithm>

namespace ns::painter
{
namespace
{
constexpr int MAX_DEPTH = 5;

static_assert(std::is_floating_point_v<Color::DataType>);

template <std::size_t N, typename T>
bool intersection_before_light_source(
        const std::optional<Intersection<N, T>>& intersection,
        const Vector<N, T>& point,
        const std::optional<T>& distance)
{
        return intersection && (!distance || (intersection->point - point).norm_squared() < square(*distance));
}

template <std::size_t N, typename T>
bool occluded(
        const Scene<N, T>& scene,
        const Vector<N, T>& geometric_normal,
        const bool use_smooth_normal,
        const Ray<N, T>& ray,
        const std::optional<T>& distance)
{
        const Vector<N, T> point = ray.org();

        std::optional<Intersection<N, T>> intersection;

        if (!use_smooth_normal || dot(ray.dir(), geometric_normal) >= 0)
        {
                // Если объект не состоит из симплексов или геометрическая сторона обращена
                // к источнику света, то напрямую рассчитать видимость источника света.
                intersection = scene.intersect(ray);
                return intersection_before_light_source(intersection, point, distance);
        }

        // Если объект состоит из симплексов и геометрическая сторона направлена
        // от источника  света, то геометрически она не освещена, но из-за нормалей
        // у вершин, дающих сглаживание, она может быть «освещена», и надо определить,
        // находится ли она в тени без учёта тени от самой поверхности в окрестности
        // точки. Это можно сделать направлением луча к источнику света с игнорированием
        // самого первого пересечения в предположении, что оно произошло с этой самой
        // окрестностью точки.

        intersection = scene.intersect(ray);
        if (!intersection_before_light_source(intersection, point, distance))
        {
                // Если луч к источнику света направлен внутрь поверхности, и нет повторного
                // пересечения с поверхностью до источника света, то нет освещения в точке.
                return true;
        }

        intersection = scene.intersect(Ray<N, T>(ray).set_org(intersection->point));
        return intersection_before_light_source(intersection, point, distance);
}

template <std::size_t N, typename T>
std::optional<Color> trace_path(
        const Scene<N, T>& scene,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        const int depth,
        RandomEngine<T>& random_engine)
{
        const std::optional<Intersection<N, T>> intersection = scene.intersect(ray);
        if (!intersection)
        {
                if (depth > 0)
                {
                        return scene.background_light_source_color();
                }
                return std::nullopt;
        }

        const Vector<N, T> v = -ray.dir();
        const Vector<N, T>& point = intersection->point;
        const SurfaceProperties surface_properties = intersection->surface->properties(point, intersection->data);
        ASSERT(surface_properties.geometric_normal.is_unit());

        const bool use_smooth_normal = smooth_normals && surface_properties.shading_normal.has_value();

        // Определять по реальной нормали, так как видимая нормаль может
        // показать, что пересечение находится с другой стороны объекта.
        const bool flip_normals = dot(v, surface_properties.geometric_normal) < 0;

        const Vector<N, T> geometric_normal =
                flip_normals ? -surface_properties.geometric_normal : surface_properties.geometric_normal;
        const Vector<N, T> n = [&]
        {
                if (use_smooth_normal)
                {
                        ASSERT(surface_properties.shading_normal->is_unit());
                        return flip_normals ? -*surface_properties.shading_normal : *surface_properties.shading_normal;
                }
                return geometric_normal;
        }();

        if (dot(n, v) <= 0)
        {
                return Color(0);
        }

        Color color_sum(0);

        if (surface_properties.light_source_color)
        {
                color_sum = *surface_properties.light_source_color;
        }

        for (const LightSource<N, T>* light_source : scene.light_sources())
        {
                const LightSourceSample<N, T> sample = light_source->sample(point);

                if (sample.color.is_black() || sample.pdf <= 0)
                {
                        continue;
                }

                const Vector<N, T>& l = sample.l;
                ASSERT(l.is_unit());

                if (dot(n, l) <= 0)
                {
                        continue;
                }

                if (occluded(scene, geometric_normal, use_smooth_normal, Ray<N, T>(point, l), sample.distance))
                {
                        continue;
                }

                const Color direct_lighting = intersection->surface->shade(point, intersection->data, n, v, l);
                color_sum += direct_lighting * sample.color / sample.pdf;
        }

        if (depth >= MAX_DEPTH)
        {
                return color_sum;
        }

        {
                const SurfaceSample<N, T> reflection = intersection->surface->sample_shade(
                        random_engine, SurfaceSampleType::Reflection, point, intersection->data, n, v);
                if (!reflection.color.is_black())
                {
                        const Vector<N, T>& l = reflection.l;
                        ASSERT(l.is_unit());
                        if (dot(l, geometric_normal) > 0)
                        {
                                const Ray<N, T> new_ray = Ray<N, T>(point, l);
                                color_sum += reflection.color
                                             * *trace_path(scene, smooth_normals, new_ray, depth + 1, random_engine);
                        }
                }
        }
        {
                const SurfaceSample<N, T> transmission = intersection->surface->sample_shade(
                        random_engine, SurfaceSampleType::Transmission, point, intersection->data, n, v);
                if (!transmission.color.is_black())
                {
                        const Vector<N, T>& l = transmission.l;
                        ASSERT(l.is_unit());
                        if (dot(l, geometric_normal) < 0)
                        {
                                const Ray<N, T> new_ray = Ray<N, T>(point, l);
                                color_sum += transmission.color
                                             * *trace_path(scene, smooth_normals, new_ray, depth + 1, random_engine);
                        }
                }
        }

        return color_sum;
}
}

template <std::size_t N, typename T>
std::optional<Color> trace_path(
        const Scene<N, T>& scene,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        RandomEngine<T>& random_engine)
{
        return trace_path(scene, smooth_normals, ray, 0 /*depth*/, random_engine);
}

#define TRACE_PATH_INSTANTIATION(N, T) \
        template std::optional<Color> trace_path(const Scene<(N), T>&, bool, const Ray<(N), T>&, RandomEngine<T>&);

TRACE_PATH_INSTANTIATION(3, float)
TRACE_PATH_INSTANTIATION(4, float)
TRACE_PATH_INSTANTIATION(5, float)
TRACE_PATH_INSTANTIATION(6, float)

TRACE_PATH_INSTANTIATION(3, double)
TRACE_PATH_INSTANTIATION(4, double)
TRACE_PATH_INSTANTIATION(5, double)
TRACE_PATH_INSTANTIATION(6, double)
}
