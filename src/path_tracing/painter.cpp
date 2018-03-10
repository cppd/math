/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "path_tracing/coefficient/cosine_sphere.h"
#include "path_tracing/sampling/sampler.h"
#include "path_tracing/sampling/sphere.h"
#include "path_tracing/space/ray_intersection.h"

#include "com/color/colors.h"
#include "com/error.h"
#include "com/random/engine.h"
#include "com/thread.h"

#include <thread>

template <unsigned N>
constexpr double DIFFUSE_LIGHT_COEFFICIENT = cosine_sphere_coefficient(N);

constexpr double MIN_COLOR_LEVEL = 1e-4;
constexpr int MAX_RECURSION_LEVEL = 100;

constexpr double DOT_PRODUCT_EPSILON = 1e-8;
constexpr double RAY_OFFSET = 1e-8;

using PainterRandomEngine = std::mt19937_64;
using PainterSampler = StratifiedJitteredSampler<2, double>;
// using PainterSampler = LatinHypercubeSampler<2, double>;

namespace
{
class Pixels
{
        class Pixel
        {
                Color m_color_sum{0};
                int m_sample_sum = 0;

        public:
                Color add_color_and_samples(const Color& color, int samples)
                {
                        m_color_sum += color;
                        m_sample_sum += samples;

                        return m_color_sum / m_sample_sum;
                }
        };

        std::vector<Pixel> m_pixels;
        const int m_width;

public:
        Pixels(int width, int height) : m_pixels(width * height), m_width(width)
        {
        }

        Color add_color_and_samples(int x, int y, const Color& color, double samples)
        {
                return m_pixels[y * m_width + x].add_color_and_samples(color, samples);
        }
};

class Counter
{
        int m_counter;

public:
        Counter(int v) : m_counter(v)
        {
        }

        operator int&()
        {
                return m_counter;
        }

        void operator=(int v)
        {
                m_counter = v;
        }

        Counter& operator=(Counter&&) = delete;
        Counter& operator=(const Counter&) = delete;
        Counter(const Counter&) = delete;
        Counter(Counter&&) = delete;
};

struct PaintData
{
        const std::vector<const GenericObject*>& objects;
        const std::vector<const LightSource*>& light_sources;
        const SurfaceProperties& default_surface_properties;

        PaintData(const std::vector<const GenericObject*>& objects_, const std::vector<const LightSource*>& light_sources_,
                  const SurfaceProperties& default_surface_properties_)
                : objects(objects_), light_sources(light_sources_), default_surface_properties(default_surface_properties_)
        {
        }
};

bool color_is_zero(const Color& c)
{
        return c.max_element() < MIN_COLOR_LEVEL;
}

// все объекты считаются непрозрачными
bool object_is_obstacle_to_light(const GenericObject* object, const ray3& ray, double distance_to_light_source)
{
        double distance_to_object;
        const Surface* surface;
        const void* intersection_data;

        if (!object->intersect_approximate(ray, &distance_to_object))
        {
                return false;
        }

        if (distance_to_object >= distance_to_light_source)
        {
                return false;
        }

        if (!object->intersect_precise(ray, distance_to_object, &distance_to_object, &surface, &intersection_data))
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

Color direct_diffuse_lighting(Counter& ray_count, const std::vector<const GenericObject*>& objects,
                              const std::vector<const LightSource*> light_sources, const vec3& p, const vec3& geometric_normal,
                              const vec3& shading_normal, bool triangle_mesh)
{
        Color color(0);

        for (const LightSource* light_source : light_sources)
        {
                Color light_source_color;
                vec3 vector_to_light;

                light_source->properties(p, &light_source_color, &vector_to_light);

                if (color_is_zero(light_source_color))
                {
                        continue;
                }

                ray3 ray_to_light = ray3(p, vector_to_light);

                double dot_light_and_normal = dot(ray_to_light.dir(), shading_normal);

                if (dot_light_and_normal <= DOT_PRODUCT_EPSILON)
                {
                        // свет находится по другую сторону поверхности
                        continue;
                }

                double light_weight = DIFFUSE_LIGHT_COEFFICIENT<3> * dot_light_and_normal;

                ++ray_count;

                ray_to_light.move_along_dir(RAY_OFFSET);

                if (!triangle_mesh || dot(ray_to_light.dir(), geometric_normal) >= 0)
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
                        const void* intersection_data;
                        if (ray_intersection(objects, ray_to_light, &t, &surface, &intersection_data))
                        {
                                double distance_to_light_source = length(vector_to_light);

                                if (t < distance_to_light_source)
                                {
                                        ++ray_count;

                                        ray_to_light.move_along_dir(t + RAY_OFFSET);

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

Color trace_path(const PaintData& paint_data, Counter& ray_count, PainterRandomEngine& random_engine, int recursion_level,
                 double color_level, const ray3& ray, bool diffuse_reflection);

Color diffuse_lighting(const PaintData& paint_data, Counter& ray_count, PainterRandomEngine& random_engine, int recursion_level,
                       double color_level, const vec3& point, const vec3& shading_normal, const vec3& geometric_normal,
                       bool triangle_mesh)
{
        if (recursion_level < MAX_RECURSION_LEVEL)
        {
                // Распределение случайного луча с вероятностью по косинусу угла между нормалью и случайным вектором.

                // Случайный вектор диффузного освещения надо определять от видимой нормали.
                ray3 diffuse_ray = ray3(point, random_cosine_weighted_on_hemisphere(random_engine, shading_normal));

                if (triangle_mesh && dot(diffuse_ray.dir(), geometric_normal) <= DOT_PRODUCT_EPSILON)
                {
                        // Если получившийся случайный вектор диффузного отражения показывает
                        // в другую сторону от поверхности, то диффузного освещения нет.
                        return Color(0);
                }

                diffuse_ray.move_along_dir(RAY_OFFSET);

                return trace_path(paint_data, ray_count, random_engine, recursion_level + 1, color_level, diffuse_ray, true);
        }

        return Color(0);
}

Color trace_path(const PaintData& paint_data, Counter& ray_count, PainterRandomEngine& random_engine, int recursion_level,
                 double color_level, const ray3& ray, bool diffuse_reflection)
{
        ++ray_count;

        const Surface* surface;
        double t;
        const void* intersection_data;

        if (!ray_intersection(paint_data.objects, ray, &t, &surface, &intersection_data))
        {
                return (paint_data.default_surface_properties.is_light_source() && diffuse_reflection) ?
                               paint_data.default_surface_properties.get_light_source_color() :
                               paint_data.default_surface_properties.get_color();
        }

        Vector point = ray.point(t);

        const SurfaceProperties surface_properties = surface->properties(point, intersection_data);

        vec3 geometric_normal = surface_properties.get_geometric_normal();

        double dot_dir_and_geometric_normal = dot(ray.dir(), geometric_normal);

        bool triangle_mesh = surface_properties.is_triangle_mesh();

        if (std::abs(dot_dir_and_geometric_normal) <= DOT_PRODUCT_EPSILON)
        {
                return Color(0);
        }

        if (surface_properties.is_light_source())
        {
                return (diffuse_reflection) ? surface_properties.get_light_source_color() : surface_properties.get_color();
        }

        vec3 shading_normal = triangle_mesh ? surface_properties.get_shading_normal() : geometric_normal;

        ASSERT(dot(geometric_normal, shading_normal) > DOT_PRODUCT_EPSILON);

        // Определять только по реальной нормали, так как видимая нормаль может
        // показать, что пересечение находится с другой стороны объекта.
        if (dot_dir_and_geometric_normal > 0)
        {
                geometric_normal = -geometric_normal;
                shading_normal = -shading_normal;
        }

        Color color(0);

        if (surface_properties.get_diffuse() > 0)
        {
                Color surface_color = surface_properties.get_diffuse() * surface_properties.get_color();

                double new_color_level = color_level * surface_color.max_element();

                if (new_color_level >= MIN_COLOR_LEVEL)
                {
                        Color direct = direct_diffuse_lighting(ray_count, paint_data.objects, paint_data.light_sources, point,
                                                               geometric_normal, shading_normal, triangle_mesh);

                        Color diffuse = diffuse_lighting(paint_data, ray_count, random_engine, recursion_level, new_color_level,
                                                         point, shading_normal, geometric_normal, triangle_mesh);

                        color += surface_color * (direct + diffuse);
                }
        }

        return color;
}

void paint_pixels(PainterRandomEngine& random_engine, std::vector<vec2>* samples, std::atomic_bool& stop,
                  const Projector& projector, const PaintData& paint_data, IPainterNotifier* painter_notifier,
                  Paintbrush* paintbrush, const PainterSampler& sampler, Pixels* pixels)
{
        std::array<int_least16_t, 2> pixel;

        Counter ray_count = 0, sample_count = 0;

        while (!stop && paintbrush->next_pixel(ray_count, sample_count, &pixel))
        {
                int x = pixel[0];
                int y = pixel[1];

                painter_notifier->painter_pixel_before(x, y);

                vec2 screen_point(x, y);

                sampler.generate(random_engine, samples);

                ray_count = 0;
                sample_count = samples->size();

                Color color(0);

                for (const vec2& sample_point : *samples)
                {
                        constexpr int recursion_level = 0;
                        constexpr double color_level = 1;
                        constexpr bool diffuse_reflection = false;

                        ray3 ray = projector.ray(screen_point + sample_point);

                        color += trace_path(paint_data, ray_count, random_engine, recursion_level, color_level, ray,
                                            diffuse_reflection);
                }

                Color pixel_color = pixels->add_color_and_samples(x, y, color, samples->size());

                painter_notifier->painter_pixel_after(x, y, pixel_color.to_srgb_integer());
        }
}

void work_thread(unsigned thread_number, ThreadBarrier& barrier, std::atomic_bool& stop, const Projector& projector,
                 const PaintData& paint_data, IPainterNotifier* painter_notifier, Paintbrush* paintbrush,
                 const PainterSampler& sampler, Pixels* pixels) noexcept
{
        try
        {
                try
                {
                        RandomEngineWithSeed<PainterRandomEngine> random_engine;

                        std::vector<vec2> samples;

                        while (true)
                        {
                                paint_pixels(random_engine, &samples, stop, projector, paint_data, painter_notifier, paintbrush,
                                             sampler, pixels);

                                barrier.wait();
                                if (stop)
                                {
                                        return;
                                }

                                if (thread_number == 0)
                                {
                                        paintbrush->next_pass();
                                }

                                barrier.wait();
                                if (stop)
                                {
                                        return;
                                }
                        }
                }
                catch (std::exception& e)
                {
                        stop = true;
                        painter_notifier->painter_error_message(std::string("Painter error:\n") + e.what());
                        barrier.wait();
                }
                catch (...)
                {
                        stop = true;
                        painter_notifier->painter_error_message("Unknown painter error");
                        barrier.wait();
                }
        }
        catch (...)
        {
                error_fatal("Exception in painter exception handlers");
        }
}

void check_thread_count(int thread_count)
{
        if (thread_count < 1)
        {
                error("Painter thread count (" + to_string(thread_count) + ") must be greater than 0");
        }
}

void check_paintbrush_projector(const Paintbrush& paintbrush, const Projector& projector)
{
        if (paintbrush.painting_width() != projector.screen_width() || paintbrush.painting_height() != projector.screen_height())
        {
                error("The paintbrush width and height (" + to_string(paintbrush.painting_width()) + ", " +
                      to_string(paintbrush.painting_height()) + ") are not equal to the projector width and height (" +
                      to_string(projector.screen_width()) + ", " + to_string(projector.screen_height()) + ")");
        }
}

void paint_threads(IPainterNotifier* painter_notifier, int samples_per_pixel, const PaintObjects& paint_objects,
                   Paintbrush* paintbrush, int thread_count, std::atomic_bool* stop)

{
        check_thread_count(thread_count);
        check_paintbrush_projector(*paintbrush, paint_objects.projector());

        const PainterSampler sampler(samples_per_pixel);

        const PaintData paint_data(paint_objects.objects(), paint_objects.light_sources(),
                                   paint_objects.default_surface_properties());

        Pixels pixels(paint_objects.projector().screen_width(), paint_objects.projector().screen_height());

        ThreadBarrier barrier(thread_count);
        std::vector<std::thread> threads(thread_count);

        paintbrush->first_pass();

        for (unsigned i = 0; i < threads.size(); ++i)
        {
                threads[i] = std::thread([&, i ]() noexcept {
                        work_thread(i, barrier, *stop, paint_objects.projector(), paint_data, painter_notifier, paintbrush,
                                    sampler, &pixels);
                });
        }

        for (std::thread& t : threads)
        {
                t.join();
        }
}
}

// Без выдачи исключений. Про проблемы сообщать через painter_notifier.
void paint(IPainterNotifier* painter_notifier, int samples_per_pixel, const PaintObjects& paint_objects, Paintbrush* paintbrush,
           int thread_count, std::atomic_bool* stop) noexcept
{
        try
        {
                try
                {
                        ASSERT(painter_notifier && paintbrush && stop);

                        paint_threads(painter_notifier, samples_per_pixel, paint_objects, paintbrush, thread_count, stop);
                }
                catch (std::exception& e)
                {
                        painter_notifier->painter_error_message(std::string("Painter error:\n") + e.what());
                }
                catch (...)
                {
                        painter_notifier->painter_error_message("Unknown painter error");
                }
        }
        catch (...)
        {
                error_fatal("Exception in painter exception handlers");
        }
}
