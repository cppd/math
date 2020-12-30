/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "painter/pixels.h"

#include <src/color/color.h>
#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/random/engine.h>
#include <src/com/thread.h>
#include <src/com/type/limit.h>
#include <src/numerical/ray.h>
#include <src/random/sj_sampler.h>
#include <src/random/sphere.h>

#include <optional>
#include <random>
#include <thread>
#include <vector>

namespace ns::painter
{
namespace
{
template <typename T>
constexpr T DOT_PRODUCT_EPSILON = 0;

constexpr Color::DataType MIN_COLOR_LEVEL = 1e-4;
constexpr int MAX_RECURSION_LEVEL = 100;
constexpr int RAY_OFFSET_IN_EPSILONS = 1000;

template <std::size_t N, typename T>
using PainterSampler = random::StratifiedJitteredSampler<N, T>;
// using PainterSampler = LatinHypercubeSampler<N, T>;

static_assert(std::is_floating_point_v<Color::DataType>);

template <std::size_t N, typename T>
struct PaintData
{
        const Scene<N, T>& scene;
        const T ray_offset;
        const bool smooth_normal;

        PaintData(const Scene<N, T>& scene, bool smooth_normal)
                : scene(scene),
                  ray_offset(scene.size() * (RAY_OFFSET_IN_EPSILONS * limits<T>::epsilon())),
                  smooth_normal(smooth_normal)
        {
        }
};

template <std::size_t N, typename T>
bool light_source_is_visible(
        int* ray_count,
        const PaintData<N, T>& paint_data,
        const Ray<N, T>& ray,
        T distance_to_light_source)
{
        ++(*ray_count);
        return !paint_data.scene.has_intersection(ray, distance_to_light_source);
}

template <std::size_t N, typename T>
struct Light
{
        Color color;
        Vector<N, T> dir_to;
        Light(const Color& color, const Vector<N, T>& dir_to) : color(color), dir_to(dir_to)
        {
        }
};

template <std::size_t N, typename T>
void find_visible_lights(
        int* ray_count,
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

                if (light_properties.color.below(MIN_COLOR_LEVEL))
                {
                        continue;
                }

                Ray<N, T> ray_to_light = Ray<N, T>(point, light_properties.direction_to_light);

                const T dot_light_and_shading_normal = dot(ray_to_light.dir(), shading_normal);

                if (dot_light_and_shading_normal <= DOT_PRODUCT_EPSILON<T>)
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

                        ++(*ray_count);
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
                                ++(*ray_count);
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
        int* ray_count,
        RandomEngine<T>& random_engine,
        int recursion_level,
        Color::DataType color_level,
        const Ray<N, T>& ray)
{
        if (recursion_level > MAX_RECURSION_LEVEL)
        {
                return std::nullopt;
        }

        ++(*ray_count);
        std::optional<Intersection<N, T>> intersection = paint_data.scene.intersect(ray);
        if (!intersection)
        {
                return std::nullopt;
        }

        Vector<N, T> point = ray.point(intersection->distance);
        const SurfaceProperties surface_properties = intersection->surface->properties(point, intersection->data);
        Vector<N, T> geometric_normal = surface_properties.geometric_normal();

        bool use_smooth_normal = paint_data.smooth_normal && surface_properties.shading_normal().has_value();

        Vector<N, T> shading_normal = use_smooth_normal ? *surface_properties.shading_normal() : geometric_normal;

        ASSERT(dot(geometric_normal, shading_normal) > 0);

        // Определять только по реальной нормали, так как видимая нормаль может
        // показать, что пересечение находится с другой стороны объекта.
        if (dot(ray.dir(), geometric_normal) > 0)
        {
                geometric_normal = -geometric_normal;
                shading_normal = -shading_normal;
        }

        Color color(0);

        if (surface_properties.light_source_color())
        {
                color += *surface_properties.light_source_color() * surface_properties.alpha();
        }

        Color::DataType reflection = surface_properties.alpha();
        if (reflection > 0)
        {
                thread_local std::vector<Light<N, T>> lights;
                find_visible_lights(
                        ray_count, paint_data, point, geometric_normal, shading_normal, use_smooth_normal, &lights);
                if (!lights.empty())
                {
                        Color direct(0);
                        for (const Light<N, T>& light : lights)
                        {
                                direct += light.color
                                          * intersection->surface->direct_lighting(
                                                  point, intersection->data, shading_normal, ray.dir(), light.dir_to);
                        }
                        color += reflection * direct;
                }

                // Случайный вектор отражения надо определять от видимой нормали.
                // Если получившийся случайный вектор отражения показывает
                // в другую сторону от поверхности, то освещения нет.

                const SurfaceReflection<N, T> surface_ray =
                        intersection->surface->reflection(point, intersection->data, shading_normal, random_engine);

                Color surface_color = reflection * surface_ray.color;
                Color::DataType new_color_level = color_level * surface_color.max_element();
                if (new_color_level >= MIN_COLOR_LEVEL
                    && (!use_smooth_normal || dot(surface_ray.direction, geometric_normal) > DOT_PRODUCT_EPSILON<T>))
                {
                        Ray<N, T> new_ray = Ray<N, T>(point, surface_ray.direction);

                        new_ray.move_along_dir(paint_data.ray_offset);

                        std::optional<Color> reflected = trace_path(
                                paint_data, ray_count, random_engine, recursion_level + 1, new_color_level, new_ray);

                        color += surface_color * reflected.value_or(paint_data.scene.background_light_source_color());
                }
        }

        Color::DataType transmission = 1 - surface_properties.alpha();
        if (transmission > 0)
        {
                Color::DataType new_color_level = color_level * transmission;
                if (new_color_level >= MIN_COLOR_LEVEL)
                {
                        Ray<N, T> new_ray = ray;
                        new_ray.set_org(point);
                        new_ray.move_along_dir(paint_data.ray_offset);

                        std::optional<Color> transmitted = trace_path(
                                paint_data, ray_count, random_engine, recursion_level + 1, new_color_level, new_ray);

                        color += transmission * transmitted.value_or(paint_data.scene.background_color());
                }
        }

        return color;
}

template <std::size_t N, typename T>
void paint_pixels(
        unsigned thread_number,
        std::atomic_bool* stop,
        const Projector<N, T>& projector,
        const PaintData<N, T>& paint_data,
        PainterNotifier<N - 1>* painter_notifier,
        Paintbrush<N - 1>* paintbrush,
        const PainterSampler<N - 1, T>& sampler,
        Pixels<N - 1>* pixels)
{
        thread_local RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
        thread_local std::vector<Vector<N - 1, T>> samples;

        std::optional<std::array<int_least16_t, N - 1>> pixel;

        samples.clear();
        int ray_count = 0;

        while (!(*stop) && (pixel = paintbrush->next_pixel(ray_count, samples.size())))
        {
                painter_notifier->painter_pixel_before(thread_number, *pixel);

                sampler.generate(random_engine, &samples);
                ray_count = 0;

                Vector<N - 1, T> screen_point = to_vector<T>(*pixel);
                int hit_sample_count = 0;
                Color color(0);

                for (const Vector<N - 1, T>& sample_point : samples)
                {
                        constexpr int RECURSION_LEVEL = 0;
                        constexpr Color::DataType COLOR_LEVEL = 1;

                        Ray<N, T> ray = projector.ray(screen_point + sample_point);

                        std::optional<Color> sample_color =
                                trace_path(paint_data, &ray_count, random_engine, RECURSION_LEVEL, COLOR_LEVEL, ray);

                        if (sample_color)
                        {
                                color += *sample_color;
                                ++hit_sample_count;
                        }
                }

                PixelInfo info = pixels->add(*pixel, color, hit_sample_count, samples.size());

                painter_notifier->painter_pixel_after(thread_number, *pixel, info.color, info.coverage);
        }
}

template <std::size_t N, typename T>
void work_thread(
        unsigned thread_number,
        ThreadBarrier* barrier,
        std::atomic_bool* stop,
        std::atomic_bool* error_caught,
        std::atomic_bool* stop_painting,
        const Projector<N, T>& projector,
        const PaintData<N, T>& paint_data,
        PainterNotifier<N - 1>* painter_notifier,
        Paintbrush<N - 1>* paintbrush,
        const PainterSampler<N - 1, T>& sampler,
        Pixels<N - 1>* pixels) noexcept
{
        try
        {
                try
                {
                        while (true)
                        {
                                paint_pixels(
                                        thread_number, stop, projector, paint_data, painter_notifier, paintbrush,
                                        sampler, pixels);

                                barrier->wait();

                                // Здесь может пройти только часть потоков из-за исключений в других
                                // потоках. Переменная *error_caught поменяться не может в других потоках,
                                // так как она в них может меняться только до барьера.
                                if (*error_caught)
                                {
                                        return;
                                }

                                // Здесь проходят все потоки

                                if (thread_number == 0)
                                {
                                        if (*stop || !paintbrush->next_pass())
                                        {
                                                *stop_painting = true;
                                        }
                                }

                                barrier->wait();

                                // Здесь проходят все потоки, а переменная *stop_painting поменяться не может
                                // в других потоках, так как она в них может меняться только до барьера.

                                if (*stop_painting)
                                {
                                        return;
                                }
                        }
                }
                catch (const std::exception& e)
                {
                        *stop = true;
                        *error_caught = true;
                        painter_notifier->painter_error_message(std::string("Painter error:\n") + e.what());
                        barrier->wait();
                }
                catch (...)
                {
                        *stop = true;
                        *error_caught = true;
                        painter_notifier->painter_error_message("Unknown painter error");
                        barrier->wait();
                }
        }
        catch (...)
        {
                error_fatal("Exception in painter exception handlers");
        }
}

template <std::size_t N, typename T>
void paint_threads(
        PainterNotifier<N - 1>* painter_notifier,
        int samples_per_pixel,
        const Scene<N, T>& scene,
        Paintbrush<N - 1>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal)
{
        if (thread_count < 1)
        {
                error("Painter thread count (" + to_string(thread_count) + ") must be greater than 0");
        }
        if (paintbrush->screen_size() != scene.projector().screen_size())
        {
                error("The paintbrush size (" + to_string(paintbrush->screen_size())
                      + ") are not equal to the projector size (" + to_string(scene.projector().screen_size()) + ")");
        }

        const PainterSampler<N - 1, T> sampler(samples_per_pixel);

        const PaintData paint_data(scene, smooth_normal);

        Pixels pixels(scene.projector().screen_size());

        ThreadBarrier barrier(thread_count);
        std::vector<std::thread> threads(thread_count);
        std::atomic_bool error_caught = false;
        std::atomic_bool stop_painting = false;

        paintbrush->first_pass();

        for (unsigned i = 0; i < threads.size(); ++i)
        {
                threads[i] = std::thread(
                        [&, i]()
                        {
                                work_thread(
                                        i, &barrier, stop, &error_caught, &stop_painting, scene.projector(), paint_data,
                                        painter_notifier, paintbrush, sampler, &pixels);
                        });
        }

        for (std::thread& t : threads)
        {
                t.join();
        }
}
}

// Без выдачи исключений. Про проблемы сообщать через painter_notifier.
template <std::size_t N, typename T>
void paint(
        PainterNotifier<N - 1>* painter_notifier,
        int samples_per_pixel,
        const Scene<N, T>& scene,
        Paintbrush<N - 1>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept
{
        try
        {
                try
                {
                        ASSERT(painter_notifier && paintbrush && stop);

                        paint_threads(
                                painter_notifier, samples_per_pixel, scene, paintbrush, thread_count, stop,
                                smooth_normal);
                }
                catch (const std::exception& e)
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

template void paint(
        PainterNotifier<2>* painter_notifier,
        int samples_per_pixel,
        const Scene<3, float>& scene,
        Paintbrush<2>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
template void paint(
        PainterNotifier<3>* painter_notifier,
        int samples_per_pixel,
        const Scene<4, float>& scene,
        Paintbrush<3>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
template void paint(
        PainterNotifier<4>* painter_notifier,
        int samples_per_pixel,
        const Scene<5, float>& scene,
        Paintbrush<4>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
template void paint(
        PainterNotifier<5>* painter_notifier,
        int samples_per_pixel,
        const Scene<6, float>& scene,
        Paintbrush<5>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;

template void paint(
        PainterNotifier<2>* painter_notifier,
        int samples_per_pixel,
        const Scene<3, double>& scene,
        Paintbrush<2>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
template void paint(
        PainterNotifier<3>* painter_notifier,
        int samples_per_pixel,
        const Scene<4, double>& scene,
        Paintbrush<3>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
template void paint(
        PainterNotifier<4>* painter_notifier,
        int samples_per_pixel,
        const Scene<5, double>& scene,
        Paintbrush<4>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
template void paint(
        PainterNotifier<5>* painter_notifier,
        int samples_per_pixel,
        const Scene<6, double>& scene,
        Paintbrush<5>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal) noexcept;
}
