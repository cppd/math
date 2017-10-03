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
#include "random_sphere.h"
#include "ray_intersection.h"

#include "com/random.h"

#include <atomic>
#include <thread>

constexpr double MINIMAL_COLOR_LEVEL = 1e-4;
constexpr int MAX_RECURSION_LEVEL = 100;

constexpr vec3 DEFAULT_COLOR = vec3(0, 0.5, 1);
constexpr vec3 DEFAULT_LIGHT_SOURCE_COLOR = vec3(1, 1, 1);
constexpr bool DEFAULT_IS_LIGHT_SOURCE = false;

namespace
{
bool color_is_zero(const vec3& c)
{
        return max_element(c) < MINIMAL_COLOR_LEVEL;
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
                     const vec3& p, const vec3& normal)
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

                double cosine_light_and_normal = dot(ray_to_light.get_dir(), normal);

                if (cosine_light_and_normal <= EPSILON)
                {
                        // свет находится по другую сторону поверхности
                        continue;
                }

                if (!light_source_is_visible(objects, ray_to_light, length(vector_to_light)))
                {
                        continue;
                }

                color += light_source_color * cosine_light_and_normal;
        }

        return color;
}

class PixelOwner
{
        PixelSequence* m_ps;
        int m_x, m_y;

public:
        PixelOwner(PixelSequence* ps, int width, int height) : m_ps(ps)
        {
                m_ps->get_pixel(&m_x, &m_y);

                if (m_x < 0 || m_y < 0 || m_x >= width || m_y >= height)
                {
                        m_ps->release_pixel(m_x, m_y);
                        error("Pixel sequence x or y coordinates out of range");
                }
        }
        ~PixelOwner()
        {
                m_ps->release_pixel(m_x, m_y);
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

class Painter : public IPainter
{
        struct Pixel
        {
                vec3 color_sum = vec3(0);
                double ray_count = 0;
        };

        class Pixels
        {
                std::vector<Pixel> m_pixels;
                unsigned m_width;

        public:
                Pixels(unsigned width, unsigned height) : m_pixels(width * height), m_width(width)
                {
                }
                Pixel* get_pixel(unsigned x, unsigned y)
                {
                        return &m_pixels[y * m_width + x];
                }
        };

        IPainterNotifier* m_painter_notifier;
        const std::vector<const GenericObject*>& m_objects;
        const std::vector<const LightSource*>& m_light_sources;
        const Projector& m_projector;
        PixelSequence* m_pixel_sequence;

        std::vector<std::thread> m_threads;
        std::atomic_bool m_stop = false;
        const std::thread::id m_window_thread_id;

        Pixels m_pixels;

        SurfaceProperties m_default_surface_properties;

        mutable std::atomic_ullong m_ray_count = 0;

public:
        Painter(IPainterNotifier* painter_notifier, const std::vector<const GenericObject*>& objects,
                const std::vector<const LightSource*>& light_sources, const Projector& projector, PixelSequence* pixel_sequence,
                unsigned thread_count)
                : m_painter_notifier(painter_notifier),
                  m_objects(objects),
                  m_light_sources(light_sources),
                  m_projector(projector),
                  m_pixel_sequence(pixel_sequence),
                  m_window_thread_id(std::this_thread::get_id()),
                  m_pixels(projector.screen_width(), projector.screen_height())

        {
                ASSERT(painter_notifier && pixel_sequence);

                if (thread_count == 0)
                {
                        error("Painter thread count = 0");
                }

                m_default_surface_properties.set_color(DEFAULT_COLOR);
                m_default_surface_properties.set_diffuse_and_fresnel(1, 0);
                m_default_surface_properties.set_light_source(DEFAULT_IS_LIGHT_SOURCE);
                m_default_surface_properties.set_light_source_color(DEFAULT_LIGHT_SOURCE_COLOR);

                for (unsigned i = 0; i < thread_count; ++i)
                {
                        m_threads.emplace_back([this]() noexcept { work_thread(); });
                }
        }

        ~Painter() override
        {
                ASSERT(std::this_thread::get_id() == m_window_thread_id);

                stop();

                for (std::thread& t : m_threads)
                {
                        if (t.joinable())
                        {
                                t.join();
                        }
                }
        }

        long long get_rays_count() const noexcept override
        {
                return m_ray_count;
        }

        void stop() noexcept
        {
                m_stop = true;
        }

        void work_thread() noexcept;

        vec3 diffuse_lighting(std::mt19937_64& random_engine, int recursion_level, double color_level, const vec3& point,
                              const vec3& normal) const;

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
                        stop();

                        std::string msg;
                        msg += "Painter error:\n";
                        msg += e.what();
                        m_painter_notifier->painter_error_message(msg);
                }
                catch (...)
                {
                        stop();

                        m_painter_notifier->painter_error_message("Unknown painter error");
                }
        }
        catch (...)
        {
                error_fatal("Exception in painter thread message string.");
        }
}

vec3 Painter::diffuse_lighting(std::mt19937_64& random_engine, int recursion_level, double color_level, const vec3& point,
                               const vec3& normal) const
{
        if (recursion_level < MAX_RECURSION_LEVEL)
        {
                ray3 ray = ray3(point, random_hemisphere_any_length(random_engine, normal));

                double cos_ray_and_normal = dot(ray.get_dir(), normal);

                if ((color_level *= cos_ray_and_normal) >= MINIMAL_COLOR_LEVEL)
                {
                        return cos_ray_and_normal * trace_path(random_engine, recursion_level + 1, color_level, ray, true);
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

        SurfaceProperties surface_properties = surface->properties(point, geometric_object);

        if (surface_properties.is_light_source())
        {
                return (diffuse_reflection) ? surface_properties.get_light_source_color() : surface_properties.get_color();
        }

        vec3 normal = surface_properties.get_normal();

        if (dot(ray.get_dir(), normal) > 0)
        {
                normal = -normal;
        }

        vec3 color(0);

        if (surface_properties.get_diffuse() > 0)
        {
                vec3 surface_color = surface_properties.get_diffuse() * surface_properties.get_color();

                double new_color_level = color_level * max_element(surface_color);

                if (new_color_level >= MINIMAL_COLOR_LEVEL)
                {
                        vec3 direct = direct_lighting(m_objects, m_light_sources, point, normal);

                        vec3 diffuse = diffuse_lighting(random_engine, recursion_level, new_color_level, point, normal);

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
                PixelOwner po(m_pixel_sequence, m_projector.screen_width(), m_projector.screen_height());

                m_painter_notifier->painter_pixel_before(po.get_x(), po.get_y());

                Pixel* pixel = m_pixels.get_pixel(po.get_x(), po.get_y());

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
                m_painter_notifier->painter_pixel_after(po.get_x(), po.get_y(), color);
        }
}
}

std::unique_ptr<IPainter> create_painter(IPainterNotifier* painter_notifier, const std::vector<const GenericObject*>& objects,
                                         const std::vector<const LightSource*>& light_sources, const Projector& projector,
                                         PixelSequence* pixel_sequence, unsigned thread_count)
{
        return std::make_unique<Painter>(painter_notifier, objects, light_sources, projector, pixel_sequence, thread_count);
}
