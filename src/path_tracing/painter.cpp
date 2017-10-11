/*
Copyright (C) 2017 Topological Manifold

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

#include "painter.h"

#include "constants.h"
#include "objects.h"
#include "ray_intersection.h"
#include "path_tracing/random/random_sphere.h"

#include "com/colors.h"
#include "com/error.h"
#include "com/random.h"

#include <atomic>
#include <thread>

constexpr double MIN_COLOR_LEVEL = 1e-4;
constexpr int MAX_RECURSION_LEVEL = 100;

namespace
{
bool color_is_zero(const vec3& c)
{
        return max_element(c) < MIN_COLOR_LEVEL;
}

// все объекты считаются непрозрачными
bool object_is_obstacle_to_light(const GenericObject* object, const ray3& ray, double distance_to_light_source)
{
        double distance_to_object;
        const Surface* surface;
        const GeometricObject* geometric_object;

        if (!object->intersect_approximate(ray, &distance_to_object))
        {
                return false;
        }

        if (distance_to_object >= distance_to_light_source)
        {
                return false;
        }

        if (!object->intersect_precise(ray, distance_to_object, &distance_to_object, &surface, &geometric_object))
        {
                return false;
        }

        if (distance_to_object >= distance_to_light_source)
        {
                return false;
        }

        return true;
}

bool light_source_is_visible(const std::vector<const GenericObject*>& objects, const ray3& ray, double distance_to_light_source)
{
        for (const GenericObject* object : objects)
        {
                if (object_is_obstacle_to_light(object, ray, distance_to_light_source))
                {
                        return false;
                }
        }
        return true;
}

vec3 direct_lighting(const std::vector<const GenericObject*>& objects, const std::vector<const LightSource*> light_sources,
                     const vec3& p, const vec3& geometric_normal, const vec3& shading_normal, bool triangle_mesh,
                     std::atomic_ullong* ray_count)
{
        vec3 color(0);

        for (const LightSource* light_source : light_sources)
        {
                vec3 light_source_color, vector_to_light;

                light_source->properties(p, &light_source_color, &vector_to_light);

                if (color_is_zero(light_source_color))
                {
                        continue;
                }

                ray3 ray_to_light = ray3(p, vector_to_light);

                double cosine_light_and_shading_normal = dot(ray_to_light.get_dir(), shading_normal);

                if (cosine_light_and_shading_normal <= EPSILON)
                {
                        // свет находится по другую сторону поверхности
                        continue;
                }

                ++(*ray_count);

                if (!triangle_mesh || dot(ray_to_light.get_dir(), geometric_normal) >= 0)
                {
                        // Если не объект из треугольников или геометрическая сторона обращена к источнику
                        // света, то напрямую рассчитать видимость источника света.
                        if (!light_source_is_visible(objects, ray_to_light, length(vector_to_light)))
                        {
                                continue;
                        }
                }
                else
                {
                        // Если объект из треугольников и геометрическая сторона направлена от источника света,
                        // то геометрически она не освещена, но из-за нормалей у вершин, дающих сглаживание,
                        // она может быть «освещена», и надо определить, находится ли она в тени без учёта
                        // тени от самой поверхности в окрестности точки. Это можно сделать направлением луча
                        // к источнику света с игнорированием самого первого пересечения в предположении, что
                        // оно произошло с этой самой окрестностью точки.
                        double t;
                        const Surface* surface;
                        const GeometricObject* geometric_object;
                        if (ray_intersection(objects, ray_to_light, &t, &surface, &geometric_object))
                        {
                                double distance_to_light_source = length(vector_to_light);

                                if (t < distance_to_light_source)
                                {
                                        ++(*ray_count);

                                        ray_to_light.set_org(ray_to_light.point(t));

                                        if (!light_source_is_visible(objects, ray_to_light, distance_to_light_source - t))
                                        {
                                                continue;
                                        }
                                }
                        }
                }

                color += light_source_color * cosine_light_and_shading_normal;
        }

        return color;
}

class PixelOwner
{
        Paintbrush* m_paintbrush;
        int m_x, m_y;

public:
        PixelOwner(Paintbrush* paintbrush, int width, int height) : m_paintbrush(paintbrush)
        {
                m_paintbrush->get_pixel(&m_x, &m_y);

                if (m_x < 0 || m_y < 0 || m_x >= width || m_y >= height)
                {
                        m_paintbrush->release_pixel(m_x, m_y);
                        error("Paintbrush x or y coordinates (" + to_string(m_x) + ", " + to_string(m_y) + ") out of range " +
                              to_string(width) + ", " + to_string(height) + ")");
                }
        }
        ~PixelOwner()
        {
                m_paintbrush->release_pixel(m_x, m_y);
        }
        int get_x() const
        {
                return m_x;
        }
        int get_y() const
        {
                return m_y;
        }
};

class Painter
{
        struct Pixel
        {
                vec3 color_sum = vec3(0);
                double ray_count = 0;
        };

        IPainterNotifier* m_painter_notifier;

        const std::vector<const GenericObject*>& m_objects;
        const std::vector<const LightSource*>& m_light_sources;
        const Projector& m_projector;
        const SurfaceProperties& m_default_surface_properties;

        Paintbrush* m_paintbrush;

        std::vector<std::thread> m_threads;
        std::atomic_bool& m_stop;

        static_assert(std::atomic_ullong::is_always_lock_free);
        std::atomic_ullong& m_ray_count;

        int m_width, m_height;
        std::vector<Pixel> m_pixels;

public:
        Painter(IPainterNotifier* painter_notifier, const PaintObjects* paint_objects, Paintbrush* paintbrush,
                unsigned thread_count, std::atomic_bool* stop, std::atomic_ullong* ray_count)
                : m_painter_notifier(painter_notifier),
                  m_objects(paint_objects->get_objects()),
                  m_light_sources(paint_objects->get_light_sources()),
                  m_projector(paint_objects->get_projector()),
                  m_default_surface_properties(paint_objects->get_default_surface_properties()),
                  m_paintbrush(paintbrush),
                  m_threads(thread_count),
                  m_stop(*stop),
                  m_ray_count(*ray_count),
                  m_width(m_projector.screen_width()),
                  m_height(m_projector.screen_height()),
                  m_pixels(m_width * m_height)
        {
                ASSERT(thread_count > 0);

                ASSERT(m_painter_notifier && paintbrush && stop && ray_count);

                m_ray_count = 0;
        }

        void process()
        {
                for (std::thread& t : m_threads)
                {
                        t = std::thread([this]() noexcept { work_thread(); });
                }

                for (std::thread& t : m_threads)
                {
                        t.join();
                }
        }

        void work_thread() noexcept;

        vec3 diffuse_lighting(std::mt19937_64& random_engine, int recursion_level, double color_level, const vec3& point,
                              const vec3& shading_normal, const vec3& geometric_normal, bool triangle_mesh) const;

        vec3 trace_path(std::mt19937_64& random_engine, int recursion_level, double color_level, const ray3& ray,
                        bool diffuse_reflection) const;

        void paint_pixels();

        Painter(const Painter&) = delete;
        Painter(Painter&&) = delete;
        Painter& operator=(const Painter&) = delete;
        Painter& operator=(Painter&&) = delete;
};

void Painter::work_thread() noexcept
{
        try
        {
                try
                {
                        paint_pixels();
                }
                catch (std::exception& e)
                {
                        m_stop = true;
                        m_painter_notifier->painter_error_message(std::string("Painter error:\n") + e.what());
                }
                catch (...)
                {
                        m_stop = true;
                        m_painter_notifier->painter_error_message("Unknown painter error");
                }
        }
        catch (...)
        {
                error_fatal("Exception in painter thread message string.");
        }
}

vec3 Painter::diffuse_lighting(std::mt19937_64& random_engine, int recursion_level, double color_level, const vec3& point,
                               const vec3& shading_normal, const vec3& geometric_normal, bool triangle_mesh) const
{
        if (recursion_level < MAX_RECURSION_LEVEL)
        {
                // Случайный вектор диффузного освещения надо определять от видимой нормали
                ray3 diffuse_ray = ray3(point, random_hemisphere_any_length(random_engine, shading_normal));

                if (triangle_mesh && dot(diffuse_ray.get_dir(), geometric_normal) < EPSILON)
                {
                        // Если получившийся случайный вектор диффузного отражения показывает
                        // в другую сторону от поверхности, то диффузного освещения нет.
                        return vec3(0);
                }

                double cos_ray_and_shading_normal = dot(diffuse_ray.get_dir(), shading_normal);

                if ((color_level *= cos_ray_and_shading_normal) >= MIN_COLOR_LEVEL)
                {
                        return cos_ray_and_shading_normal *
                               trace_path(random_engine, recursion_level + 1, color_level, diffuse_ray, true);
                }
        }

        return vec3(0);
}

vec3 Painter::trace_path(std::mt19937_64& random_engine, int recursion_level, double color_level, const ray3& ray,
                         bool diffuse_reflection) const
{
        ++m_ray_count;

        const Surface* surface;
        double t;
        const GeometricObject* geometric_object;

        if (!ray_intersection(m_objects, ray, &t, &surface, &geometric_object))
        {
                return (m_default_surface_properties.is_light_source() && diffuse_reflection) ?
                               m_default_surface_properties.get_light_source_color() :
                               m_default_surface_properties.get_color();
        }

        Vector point = ray.point(t);
        const SurfaceProperties surface_properties = surface->properties(point, geometric_object);
        vec3 geometric_normal = surface_properties.get_geometric_normal();
        double dot_dir_and_geometric_normal = dot(ray.get_dir(), geometric_normal);
        bool triangle_mesh = surface_properties.is_triangle_mesh();

        if (std::abs(dot_dir_and_geometric_normal) <= EPSILON)
        {
                return vec3(0);
        }

        if (surface_properties.is_light_source())
        {
                return (diffuse_reflection) ? surface_properties.get_light_source_color() : surface_properties.get_color();
        }

        vec3 shading_normal = triangle_mesh ? surface_properties.get_shading_normal() : geometric_normal;

        ASSERT(dot(geometric_normal, shading_normal) > EPSILON);

        // Определять только по реальной нормали, так как видимая нормаль может
        // показать, что пересечение находится с другой стороны объекта.
        if (dot_dir_and_geometric_normal > EPSILON)
        {
                geometric_normal = -geometric_normal;
                shading_normal = -shading_normal;
        }

        vec3 color(0);

        if (surface_properties.get_diffuse() > 0)
        {
                vec3 surface_color = surface_properties.get_diffuse() * surface_properties.get_color();

                double new_color_level = color_level * max_element(surface_color);

                if (new_color_level >= MIN_COLOR_LEVEL)
                {
                        vec3 direct = direct_lighting(m_objects, m_light_sources, point, geometric_normal, shading_normal,
                                                      triangle_mesh, &m_ray_count);

                        vec3 diffuse = diffuse_lighting(random_engine, recursion_level, new_color_level, point, shading_normal,
                                                        geometric_normal, triangle_mesh);

                        color += surface_color * (direct + diffuse);
                }
        }

        return color;
}

void Painter::paint_pixels()
{
        const int pixel_resolution = m_projector.pixel_resolution();
        const double pixel_step = 1.0 / pixel_resolution;
        const int pixel_ray_count = square(m_projector.pixel_resolution());

        std::mt19937_64 random_engine(get_random_seed<std::mt19937_64>());
        std::uniform_real_distribution<double> urd(0, pixel_step);

        while (!m_stop)
        {
                PixelOwner po(m_paintbrush, m_projector.screen_width(), m_projector.screen_height());

                m_painter_notifier->painter_pixel_before(po.get_x(), po.get_y());

                Pixel* pixel = &m_pixels[po.get_y() * m_width + po.get_x()];

                vec2 point;
                int i, j;
                for (i = 0, point[0] = po.get_x(); i < pixel_resolution; ++i, point[0] += pixel_step)
                {
                        for (j = 0, point[1] = po.get_y(); j < pixel_resolution; ++j, point[1] += pixel_step)
                        {
                                vec2 screen_point = point + vec2(urd(random_engine), urd(random_engine));

                                vec3 color = trace_path(random_engine, 0 /*recursion level*/, 1 /*color level*/,
                                                        m_projector.ray(screen_point), false /*diffuse reflection*/);

                                pixel->color_sum += color;
                        }
                }

                pixel->ray_count += pixel_ray_count;

                vec3 color = pixel->color_sum / pixel->ray_count;
                unsigned char r = rgb_float_to_srgb_int8(color[0]);
                unsigned char g = rgb_float_to_srgb_int8(color[1]);
                unsigned char b = rgb_float_to_srgb_int8(color[2]);
                m_painter_notifier->painter_pixel_after(po.get_x(), po.get_y(), r, g, b);
        }
}
}

void paint(IPainterNotifier* painter_notifier, const PaintObjects* paint_objects, Paintbrush* paintbrush, unsigned thread_count,
           std::atomic_bool* stop, std::atomic_ullong* ray_count)
{
        Painter(painter_notifier, paint_objects, paintbrush, thread_count, stop, ray_count).process();
}
