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

#include <vector>

namespace ns::painter
{
namespace
{
constexpr int MAX_DEPTH = 5;

static_assert(std::is_floating_point_v<Color::DataType>);

template <std::size_t N, typename T>
bool intersection_before_light_source(
        const std::optional<Intersection<N, T>>& intersection,
        const std::optional<T> distance_to_light)
{
        return intersection && (!distance_to_light || intersection->distance < *distance_to_light);
}

template <std::size_t N, typename T>
bool occluded(
        const Scene<N, T>& scene,
        const T ray_offset,
        const Vector<N, T>& geometric_normal,
        const bool use_smooth_normal,
        Ray<N, T> ray_to_light,
        std::optional<T> distance_to_light)
{
        if (!use_smooth_normal || dot(ray_to_light.dir(), geometric_normal) >= 0)
        {
                // Если объект не состоит из симплексов или геометрическая сторона обращена
                // к источнику света, то напрямую рассчитать видимость источника света.
                ray_to_light.move_along_dir(ray_offset);
                if (distance_to_light)
                {
                        *distance_to_light -= ray_offset;
                }
                std::optional<Intersection<N, T>> intersection = scene.intersect(ray_to_light);
                return intersection_before_light_source(intersection, distance_to_light);
        }

        // Если объект состоит из симплексов и геометрическая сторона направлена
        // от источника  света, то геометрически она не освещена, но из-за нормалей
        // у вершин, дающих сглаживание, она может быть «освещена», и надо определить,
        // находится ли она в тени без учёта тени от самой поверхности в окрестности
        // точки. Это можно сделать направлением луча к источнику света с игнорированием
        // самого первого пересечения в предположении, что оно произошло с этой самой
        // окрестностью точки.

        ray_to_light.move_along_dir(ray_offset);
        if (distance_to_light)
        {
                *distance_to_light -= ray_offset;
        }
        std::optional<Intersection<N, T>> intersection = scene.intersect(ray_to_light);
        if (!intersection_before_light_source(intersection, distance_to_light))
        {
                // Если луч к источнику света направлен внутрь поверхности, и нет повторного
                // пересечения с поверхностью до источника света, то нет освещения в точке.
                return true;
        }

        //{
        //        Ray<N, T> ray_from_light = ray_to_light.reverse_ray();
        //        ray_from_light.move_along_dir(2 * ray_offset);
        //        std::optional<Intersection<N, T>> from_light = scene.intersect(ray_from_light);
        //        if (from_light && from_light->distance < intersection->distance)
        //        {
        //                // Если для луча, направленного от поверхности и от источника света,
        //                // имеется пересечение с поверхностью на расстоянии меньше, чем расстояние
        //                // до пересечения внутрь поверхности к источнику света, то предполагается,
        //                // что точка находится по другую сторону от источника света.
        //                return true;
        //        }
        //}

        ray_to_light.move_along_dir(intersection->distance);
        ray_to_light.move_along_dir(ray_offset);
        if (distance_to_light)
        {
                *distance_to_light -= intersection->distance;
                *distance_to_light -= ray_offset;
        }
        intersection = scene.intersect(ray_to_light);
        return intersection_before_light_source(intersection, distance_to_light);
}

template <std::size_t N, typename T>
std::optional<Color> trace_path(
        const Scene<N, T>& scene,
        const T ray_offset,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        const int depth,
        RandomEngine<T>& random_engine)
{
        std::optional<Intersection<N, T>> intersection = scene.intersect(ray);
        if (!intersection)
        {
                if (depth > 0)
                {
                        return scene.background_light_source_color();
                }
                return std::nullopt;
        }

        Vector<N, T> point = ray.point(intersection->distance);
        const SurfaceProperties surface_properties = intersection->surface->properties(point, intersection->data);

        bool use_smooth_normal = smooth_normals && surface_properties.shading_normal().has_value();

        Vector<N, T> geometric_normal = surface_properties.geometric_normal();
        Vector<N, T> n = use_smooth_normal ? *surface_properties.shading_normal() : geometric_normal;
        if (dot(geometric_normal, n) <= 0)
        {
                return Color(0);
        }

        const Vector<N, T> v = -ray.dir();

        // Определять только по реальной нормали, так как видимая нормаль может
        // показать, что пересечение находится с другой стороны объекта.
        if (dot(v, geometric_normal) < 0)
        {
                geometric_normal = -geometric_normal;
                n = -n;
        }

        if (dot(v, n) <= 0)
        {
                return Color(0);
        }

        Color color_sum(0);

        const Color::DataType alpha = surface_properties.alpha();

        if (alpha > 0)
        {
                if (surface_properties.light_source_color())
                {
                        color_sum = *surface_properties.light_source_color();
                }

                for (const LightSource<N, T>* light_source : scene.light_sources())
                {
                        const LightSourceSample<N, T> sample = light_source->sample(point);

                        if (sample.color.is_black() || sample.pdf <= 0)
                        {
                                continue;
                        }

                        const Ray<N, T> ray_to_light = Ray<N, T>(point, sample.l);
                        const Vector<N, T> l = ray_to_light.dir();
                        if (dot(l, n) <= 0)
                        {
                                continue;
                        }

                        if (occluded(
                                    scene, ray_offset, geometric_normal, use_smooth_normal, ray_to_light,
                                    sample.distance))
                        {
                                continue;
                        }

                        Color surface_color = intersection->surface->shade(point, intersection->data, n, v, l);
                        color_sum += surface_color * sample.color / sample.pdf;
                }

                color_sum *= alpha;
        }

        if (depth >= MAX_DEPTH)
        {
                return color_sum;
        }

        if (alpha > 0)
        {
                // Случайный вектор отражения надо определять от видимой нормали.
                // Если получившийся случайный вектор отражения показывает
                // в другую сторону от поверхности, то освещения нет.

                SurfaceReflection<N, T> reflection =
                        intersection->surface->reflect(random_engine, point, intersection->data, n, v);
                if (!reflection.color.is_black() && dot(reflection.l, geometric_normal) > 0)
                {
                        Ray<N, T> new_ray = Ray<N, T>(point, reflection.l);
                        new_ray.move_along_dir(ray_offset);

                        Color reflected =
                                *trace_path(scene, ray_offset, smooth_normals, new_ray, depth + 1, random_engine);

                        color_sum += alpha * reflection.color * reflected;
                }
        }

        if (alpha < 1)
        {
                Ray<N, T> new_ray = ray;
                new_ray.set_org(point);
                new_ray.move_along_dir(ray_offset);

                Color transmitted = *trace_path(scene, ray_offset, smooth_normals, new_ray, depth + 1, random_engine);

                color_sum += (1 - alpha) * transmitted;
        }

        return color_sum;
}
}

template <std::size_t N, typename T>
std::optional<Color> trace_path(
        const Scene<N, T>& scene,
        const T ray_offset,
        const bool smooth_normals,
        const Ray<N, T>& ray,
        RandomEngine<T>& random_engine)
{
        return trace_path(scene, ray_offset, smooth_normals, ray, 0 /*depth*/, random_engine);
}

#define CREATE_TRACE_PATH_INSTANTIATION(N, T) \
        template std::optional<Color> trace_path(const Scene<(N), T>&, T, bool, const Ray<(N), T>&, RandomEngine<T>&);

CREATE_TRACE_PATH_INSTANTIATION(3, float)
CREATE_TRACE_PATH_INSTANTIATION(4, float)
CREATE_TRACE_PATH_INSTANTIATION(5, float)
CREATE_TRACE_PATH_INSTANTIATION(6, float)

CREATE_TRACE_PATH_INSTANTIATION(3, double)
CREATE_TRACE_PATH_INSTANTIATION(4, double)
CREATE_TRACE_PATH_INSTANTIATION(5, double)
CREATE_TRACE_PATH_INSTANTIATION(6, double)
}
