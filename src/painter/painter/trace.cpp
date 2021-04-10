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

class RayCount final
{
        int m_count = 0;

public:
        void inc()
        {
                ++m_count;
        }

        int value() const
        {
                return m_count;
        }
};

template <std::size_t N, typename T>
bool light_source_is_visible(
        RayCount* ray_count,
        const PaintData<N, T>& paint_data,
        const Ray<N, T>& ray,
        T distance_to_light_source)
{
        ray_count->inc();
        return !paint_data.scene.has_intersection(ray, distance_to_light_source);
}

template <std::size_t N, typename T>
struct Light final
{
        Color color;
        Vector<N, T> l;

        Light(const Color& color, const Vector<N, T>& l) : color(color), l(l)
        {
        }
};

template <std::size_t N, typename T>
void find_visible_lights(
        RayCount* ray_count,
        const PaintData<N, T>& paint_data,
        const Vector<N, T>& point,
        const Vector<N, T>& geometric_normal,
        const Vector<N, T>& shading_normal,
        bool use_smooth_normal,
        std::vector<Light<N, T>>* lights)
{
        lights->clear();

        for (const LightSource<N, T>* light_source : paint_data.scene.light_sources())
        {
                const LightProperties light_properties = light_source->properties(point);

                if (light_properties.color.is_black())
                {
                        continue;
                }

                Ray<N, T> ray_to_light = Ray<N, T>(point, light_properties.direction_to_light);

                const T dot_light_and_shading_normal = dot(ray_to_light.dir(), shading_normal);

                if (dot_light_and_shading_normal <= 0)
                {
                        // Свет находится по другую сторону поверхности
                        continue;
                }

                if (!use_smooth_normal || dot(ray_to_light.dir(), geometric_normal) >= 0)
                {
                        // Если объект не состоит из симплексов или геометрическая сторона обращена
                        // к источнику света, то напрямую рассчитать видимость источника света.

                        ray_to_light.move_along_dir(paint_data.ray_offset);
                        if (!light_source_is_visible(
                                    ray_count, paint_data, ray_to_light, light_properties.direction_to_light.norm()))
                        {
                                continue;
                        }
                }
                else
                {
                        // Если объект состоит из симплексов и геометрическая сторона направлена
                        // от источника  света, то геометрически она не освещена, но из-за нормалей
                        // у вершин, дающих сглаживание, она может быть «освещена», и надо определить,
                        // находится ли она в тени без учёта тени от самой поверхности в окрестности
                        // точки. Это можно сделать направлением луча к источнику света с игнорированием
                        // самого первого пересечения в предположении, что оно произошло с этой самой
                        // окрестностью точки.

                        ray_count->inc();
                        ray_to_light.move_along_dir(paint_data.ray_offset);
                        std::optional<Intersection<N, T>> intersection = paint_data.scene.intersect(ray_to_light);
                        if (!intersection)
                        {
                                // Если луч к источнику света направлен внутрь поверхности, и нет повторного
                                // пересечения с поверхностью, то нет освещения в точке.
                                continue;
                        }

                        T distance_to_light_source = light_properties.direction_to_light.norm();

                        if (intersection->distance >= distance_to_light_source)
                        {
                                // Источник света находится внутри поверхности
                                continue;
                        }

                        {
                                ray_count->inc();
                                Ray<N, T> ray_from_light = ray_to_light.reverse_ray();
                                ray_from_light.move_along_dir(2 * paint_data.ray_offset);
                                std::optional<Intersection<N, T>> from_light =
                                        paint_data.scene.intersect(ray_from_light);
                                if (from_light && (from_light->distance < intersection->distance))
                                {
                                        // Если для луча, направленного от поверхности и от источника света,
                                        // имеется пересечение с поверхностью на расстоянии меньше, чем расстояние
                                        // до пересечения внутрь поверхности к источнику света, то предполагается,
                                        // что точка находится по другую сторону от источника света.
                                        continue;
                                }
                        }

                        ray_to_light.move_along_dir(intersection->distance + paint_data.ray_offset);
                        if (!light_source_is_visible(
                                    ray_count, paint_data, ray_to_light,
                                    distance_to_light_source - intersection->distance))
                        {
                                continue;
                        }
                }

                lights->emplace_back(light_properties.color, ray_to_light.dir());
        }
}

template <std::size_t N, typename T>
std::optional<Color> trace_path(
        const PaintData<N, T>& paint_data,
        RayCount* ray_count,
        RandomEngine<T>& random_engine,
        int depth,
        const Ray<N, T>& ray)
{
        ray_count->inc();
        std::optional<Intersection<N, T>> intersection = paint_data.scene.intersect(ray);
        if (!intersection)
        {
                if (depth > 0)
                {
                        return paint_data.scene.background_light_source_color();
                }
                return std::nullopt;
        }

        Vector<N, T> point = ray.point(intersection->distance);
        const SurfaceProperties surface_properties = intersection->surface->properties(point, intersection->data);

        bool use_smooth_normal = paint_data.smooth_normal && surface_properties.shading_normal().has_value();

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

                thread_local std::vector<Light<N, T>> lights;

                find_visible_lights(ray_count, paint_data, point, geometric_normal, n, use_smooth_normal, &lights);

                for (const Light<N, T>& light : lights)
                {
                        Color color = intersection->surface->lighting(point, intersection->data, n, v, light.l);
                        color_sum += color * light.color;
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

                Reflection<N, T> reflection =
                        intersection->surface->reflection(random_engine, point, intersection->data, n, v);
                if (!reflection.color.is_black() && dot(reflection.l, geometric_normal) > 0)
                {
                        Ray<N, T> new_ray = Ray<N, T>(point, reflection.l);
                        new_ray.move_along_dir(paint_data.ray_offset);

                        Color reflected = *trace_path(paint_data, ray_count, random_engine, depth + 1, new_ray);

                        color_sum += alpha * reflection.color * reflected;
                }
        }

        if (alpha < 1)
        {
                Ray<N, T> new_ray = ray;
                new_ray.set_org(point);
                new_ray.move_along_dir(paint_data.ray_offset);

                Color transmitted = *trace_path(paint_data, ray_count, random_engine, depth + 1, new_ray);

                color_sum += (1 - alpha) * transmitted;
        }

        return color_sum;
}
}

template <std::size_t N, typename T>
TraceResult trace_path(const PaintData<N, T>& paint_data, const Ray<N, T>& ray, RandomEngine<T>& random_engine)
{
        RayCount ray_count;

        TraceResult result;
        result.color = trace_path(paint_data, &ray_count, random_engine, 0 /*depth*/, ray);
        result.ray_count = ray_count.value();
        return result;
}

#define CREATE_TRACE_PATH_INSTANTIATION(N, T) \
        template TraceResult trace_path(const PaintData<(N), T>&, const Ray<(N), T>&, RandomEngine<T>&);

CREATE_TRACE_PATH_INSTANTIATION(3, float)
CREATE_TRACE_PATH_INSTANTIATION(4, float)
CREATE_TRACE_PATH_INSTANTIATION(5, float)
CREATE_TRACE_PATH_INSTANTIATION(6, float)

CREATE_TRACE_PATH_INSTANTIATION(3, double)
CREATE_TRACE_PATH_INSTANTIATION(4, double)
CREATE_TRACE_PATH_INSTANTIATION(5, double)
CREATE_TRACE_PATH_INSTANTIATION(6, double)
}
