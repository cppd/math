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
#include "path_tracing/random/random_vector.h"
#include "path_tracing/space/ray_intersection.h"

#include "com/colors.h"
#include "com/error.h"
#include "com/random.h"

#include <atomic>
#include <thread>

constexpr double MIN_COLOR_LEVEL = 1e-4;
constexpr int MAX_RECURSION_LEVEL = 100;

constexpr double EPSILON_DOUBLE = EPSILON<double>;

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

vec3 direct_diffuse_lighting(const std::vector<const GenericObject*>& objects,
                             const std::vector<const LightSource*> light_sources, const vec3& p, const vec3& geometric_normal,
                             const vec3& shading_normal, bool triangle_mesh, AtomicCounter<unsigned long long>* ray_count)
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

                double light_weight = 2 * dot(ray_to_light.get_dir(), shading_normal);

                if (light_weight <= EPSILON_DOUBLE)
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

                color += light_source_color * light_weight;
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

class Pixel
{
        vec3 m_color_sum = vec3(0);
        double m_ray_sum = 0;

public:
        void add_color(const vec3& color)
        {
                m_color_sum += color;
        }
        void add_rays(double rays)
        {
                m_ray_sum += rays;
        }
        vec3 color() const
        {
                return m_color_sum / m_ray_sum;
        }
};

class Painter
{
        IPainterNotifier* m_painter_notifier;

        const std::vector<const GenericObject*>& m_objects;
        const std::vector<const LightSource*>& m_light_sources;
        const Projector& m_projector;
        const Sampler& m_sampler;
        const SurfaceProperties& m_default_surface_properties;

        Paintbrush* m_paintbrush;

        std::vector<std::thread> m_threads;
        std::atomic_bool& m_stop;

        static_assert(AtomicCounter<unsigned long long>::is_always_lock_free);
        AtomicCounter<unsigned long long>& m_ray_count;
        AtomicCounter<unsigned long long>& m_sample_count;

        int m_width, m_height;
        std::vector<Pixel> m_pixels;

        Pixel& get_pixel_reference(int x, int y);
        void work_thread() noexcept;
        vec3 diffuse_lighting(std::mt19937_64& random_engine, int recursion_level, double color_level, const vec3& point,
                              const vec3& shading_normal, const vec3& geometric_normal, bool triangle_mesh) const;
        vec3 trace_path(std::mt19937_64& random_engine, int recursion_level, double color_level, const ray3& ray,
                        bool diffuse_reflection) const;
        void paint_pixels();

public:
        Painter(IPainterNotifier* painter_notifier, const PaintObjects& paint_objects, Paintbrush* paintbrush,
                unsigned thread_count, std::atomic_bool* stop, AtomicCounter<unsigned long long>* ray_count,
                AtomicCounter<unsigned long long>* sample_count)
                : m_painter_notifier(painter_notifier),
                  m_objects(paint_objects.objects()),
                  m_light_sources(paint_objects.light_sources()),
                  m_projector(paint_objects.projector()),
                  m_sampler(paint_objects.sampler()),
                  m_default_surface_properties(paint_objects.default_surface_properties()),
                  m_paintbrush(paintbrush),
                  m_threads(thread_count),
                  m_stop(*stop),
                  m_ray_count(*ray_count),
                  m_sample_count(*sample_count),
                  m_width(m_projector.screen_width()),
                  m_height(m_projector.screen_height()),
                  m_pixels(m_width * m_height)
        {
                if (thread_count < 1)
                {
                        error("Painter thread count (" + to_string(thread_count) + ") must be greater than 0");
                }
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
                // Распределение случайного луча с вероятностью по косинусу угла между нормалью и случайным вектором.

                // Случайный вектор диффузного освещения надо определять от видимой нормали.
                ray3 diffuse_ray = ray3(point, random_hemisphere_cosine_any_length(random_engine, shading_normal));

                if (triangle_mesh && dot(diffuse_ray.get_dir(), geometric_normal) < EPSILON_DOUBLE)
                {
                        // Если получившийся случайный вектор диффузного отражения показывает
                        // в другую сторону от поверхности, то диффузного освещения нет.
                        return vec3(0);
                }

                return trace_path(random_engine, recursion_level + 1, color_level, diffuse_ray, true);
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

        if (std::abs(dot_dir_and_geometric_normal) <= EPSILON_DOUBLE)
        {
                return vec3(0);
        }

        if (surface_properties.is_light_source())
        {
                return (diffuse_reflection) ? surface_properties.get_light_source_color() : surface_properties.get_color();
        }

        vec3 shading_normal = triangle_mesh ? surface_properties.get_shading_normal() : geometric_normal;

        ASSERT(dot(geometric_normal, shading_normal) > EPSILON_DOUBLE);

        // Определять только по реальной нормали, так как видимая нормаль может
        // показать, что пересечение находится с другой стороны объекта.
        if (dot_dir_and_geometric_normal > EPSILON_DOUBLE)
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
                        vec3 direct = direct_diffuse_lighting(m_objects, m_light_sources, point, geometric_normal, shading_normal,
                                                              triangle_mesh, &m_ray_count);

                        vec3 diffuse = diffuse_lighting(random_engine, recursion_level, new_color_level, point, shading_normal,
                                                        geometric_normal, triangle_mesh);

                        color += surface_color * (direct + diffuse);
                }
        }

        return color;
}

Pixel& Painter::get_pixel_reference(int x, int y)
{
        return m_pixels[y * m_width + x];
}

void Painter::paint_pixels()
{
        std::mt19937_64 random_engine(get_random_seed<std::mt19937_64>());

        std::vector<vec2> samples;

        while (!m_stop)
        {
                PixelOwner po(m_paintbrush, m_width, m_height);

                int x = po.get_x();
                int y = po.get_y();

                m_painter_notifier->painter_pixel_before(x, y);

                Pixel& pixel = get_pixel_reference(x, y);
                vec2 screen_point(x, y);

                m_sampler.generate(random_engine, &samples);

                for (const vec2& sample_point : samples)
                {
                        constexpr int recursion_level = 0;
                        constexpr double color_level = 1;
                        constexpr bool diffuse_reflection = false;

                        ray3 ray = m_projector.ray(screen_point + sample_point);

                        vec3 color = trace_path(random_engine, recursion_level, color_level, ray, diffuse_reflection);

                        pixel.add_color(color);
                }

                pixel.add_rays(samples.size());

                m_painter_notifier->painter_pixel_after(x, y, rgb_float_to_srgb_int8(pixel.color()));

                m_sample_count += samples.size();
        }
}
}

// Без выдачи исключений. Про проблемы сообщать через painter_notifier.
void paint(IPainterNotifier* painter_notifier, const PaintObjects& paint_objects, Paintbrush* paintbrush, unsigned thread_count,
           std::atomic_bool* stop, AtomicCounter<unsigned long long>* ray_count,
           AtomicCounter<unsigned long long>* sample_count) noexcept
{
        ASSERT(painter_notifier && paintbrush && stop && ray_count);

        try
        {
                try
                {
                        Painter p(painter_notifier, paint_objects, paintbrush, thread_count, stop, ray_count, sample_count);
                        p.process();
                }
                catch (std::exception& e)
                {
                        painter_notifier->painter_error_message(std::string("Painter class thread error:\n") + e.what());
                }
                catch (...)
                {
                        painter_notifier->painter_error_message("Unknown painter class thread error");
                }
        }
        catch (...)
        {
                error_fatal("Exception in painter class thread message string.");
        }
}
