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

#include "coefficient/cosine_sphere.h"
#include "sampling/samplers.h"
#include "sampling/sphere.h"

#include <src/color/color.h>
#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/thread.h>
#include <src/com/type/limit.h>
#include <src/numerical/ray.h>
#include <src/utility/random/engine.h>

#include <optional>
#include <random>
#include <thread>
#include <vector>

namespace painter
{
namespace
{
template <size_t N, typename T>
constexpr T DIFFUSE_LIGHT_COEFFICIENT = cosine_sphere_coefficient(N);

template <typename T>
constexpr T DOT_PRODUCT_EPSILON = 0;

constexpr Color::DataType MIN_COLOR_LEVEL = 1e-4;

constexpr int MAX_RECURSION_LEVEL = 100;

constexpr int RAY_OFFSET_IN_EPSILONS = 1000;

template <typename T>
using PainterRandomEngine = std::conditional_t<std::is_same_v<std::remove_cv<T>, float>, std::mt19937, std::mt19937_64>;

template <size_t N, typename T>
using PainterSampler = StratifiedJitteredSampler<N, T>;
// using PainterSampler = LatinHypercubeSampler<N, T>;

static_assert(std::is_floating_point_v<Color::DataType>);

template <size_t N>
class Pixels final
{
        using CounterType = std::uint_least16_t;

        class Pixel
        {
                Color m_color_sum{0};
                CounterType m_hit_sample_sum = 0;
                CounterType m_all_sample_sum = 0;

        public:
                void add_color_and_samples(
                        const Color& color,
                        CounterType hit_samples,
                        CounterType all_samples,
                        Color* result_color,
                        float* coverage)
                {
                        m_all_sample_sum += all_samples;
                        m_hit_sample_sum += hit_samples;
                        *coverage = static_cast<float>(m_hit_sample_sum) / m_all_sample_sum;
                        m_color_sum += color;
                        *result_color = m_hit_sample_sum > 0 ? m_color_sum / m_hit_sample_sum : m_color_sum;
                }
        };

        const GlobalIndex<N, long long> m_global_index;
        std::vector<Pixel> m_pixels;

public:
        explicit Pixels(const std::array<int, N>& screen_size) : m_global_index(screen_size)
        {
                m_pixels.resize(m_global_index.count());
        }

        void add_color_and_samples(
                const std::array<int_least16_t, N>& pixel,
                const Color& color,
                CounterType hit_samples,
                CounterType all_samples,
                Color* result_color,
                float* coverage)
        {
                m_pixels[m_global_index.compute(pixel)].add_color_and_samples(
                        color, hit_samples, all_samples, result_color, coverage);
        }
};

class Counter final
{
        int m_counter = 0;

public:
        Counter() = default;

        Counter& operator=(const Counter&) = delete;
        Counter& operator=(Counter&&) = delete;
        Counter(const Counter&) = delete;
        Counter(Counter&&) = delete;

        void reset()
        {
                m_counter = 0;
        }

        void inc()
        {
                ++m_counter;
        }

        int value() const
        {
                return m_counter;
        }
};

template <size_t N, typename T>
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

bool color_is_zero(const Color& c)
{
        return c.max_element() < MIN_COLOR_LEVEL;
}

template <size_t N, typename T>
bool light_source_is_visible(
        Counter* ray_count,
        const PaintData<N, T>& paint_data,
        const Ray<N, T>& ray,
        T distance_to_light_source)
{
        ray_count->inc();
        return !paint_data.scene.has_intersection(ray, distance_to_light_source);
}

template <size_t N, typename T>
Color direct_diffuse_lighting(
        Counter* ray_count,
        const PaintData<N, T>& paint_data,
        const Vector<N, T>& p,
        const Vector<N, T>& geometric_normal,
        const Vector<N, T>& shading_normal,
        bool smooth_normal,
        const T& ray_offset)
{
        Color color(0);

        for (const LightSource<N, T>* light_source : paint_data.scene.light_sources())
        {
                const LightProperties light_properties = light_source->properties(p);

                if (color_is_zero(light_properties.color))
                {
                        continue;
                }

                Ray<N, T> ray_to_light = Ray<N, T>(p, light_properties.direction_to_light);

                const T dot_light_and_normal = dot(ray_to_light.dir(), shading_normal);

                if (dot_light_and_normal <= DOT_PRODUCT_EPSILON<T>)
                {
                        // Свет находится по другую сторону поверхности
                        continue;
                }

                if (!smooth_normal || dot(ray_to_light.dir(), geometric_normal) >= 0)
                {
                        // Если объект не состоит из симплексов или геометрическая сторона обращена
                        // к источнику света, то напрямую рассчитать видимость источника света.

                        ray_to_light.move_along_dir(ray_offset);
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
                        ray_to_light.move_along_dir(ray_offset);
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
                                ray_from_light.move_along_dir(2 * ray_offset);
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

                        ray_to_light.move_along_dir(intersection->distance + ray_offset);
                        if (!light_source_is_visible(
                                    ray_count, paint_data, ray_to_light,
                                    distance_to_light_source - intersection->distance))
                        {
                                continue;
                        }
                }

                T light_weight = DIFFUSE_LIGHT_COEFFICIENT<N, T> * dot_light_and_normal;

                color += light_properties.color * light_weight;
        }

        return color;
}

template <size_t N, typename T>
bool diffuse_weighted_ray(
        const PaintData<N, T>& paint_data,
        PainterRandomEngine<T>& random_engine,
        const Vector<N, T>& point,
        const Vector<N, T>& shading_normal,
        const Vector<N, T>& geometric_normal,
        bool smooth_normal,
        Ray<N, T>* ray)
{
        // Распределение случайного луча с вероятностью по косинусу угла между нормалью и случайным вектором.

        // Случайный вектор диффузного освещения надо определять от видимой нормали.
        *ray = Ray<N, T>(point, random_cosine_weighted_on_hemisphere(random_engine, shading_normal));

        if (smooth_normal && dot(ray->dir(), geometric_normal) <= DOT_PRODUCT_EPSILON<T>)
        {
                // Если получившийся случайный вектор диффузного отражения показывает
                // в другую сторону от поверхности, то диффузного освещения нет.
                return false;
        }

        ray->move_along_dir(paint_data.ray_offset);

        return true;
}

template <size_t N, typename T>
std::optional<Color> trace_path(
        const PaintData<N, T>& paint_data,
        Counter* ray_count,
        PainterRandomEngine<T>& random_engine,
        int recursion_level,
        Color::DataType color_level,
        const Ray<N, T>& ray)
{
        if (recursion_level > MAX_RECURSION_LEVEL)
        {
                return std::nullopt;
        }

        ray_count->inc();
        std::optional<Intersection<N, T>> intersection = paint_data.scene.intersect(ray);
        if (!intersection)
        {
                return std::nullopt;
        }

        Vector<N, T> point = ray.point(intersection->distance);
        const SurfaceProperties surface_properties = intersection->surface->properties(point, intersection->data);
        Vector<N, T> geometric_normal = surface_properties.geometric_normal();

        bool smooth_normal = paint_data.smooth_normal && surface_properties.shading_normal().has_value();

        Vector<N, T> shading_normal = smooth_normal ? *surface_properties.shading_normal() : geometric_normal;

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

        Color::DataType reflection = surface_properties.diffuse() * surface_properties.alpha();
        if (reflection > 0)
        {
                Color surface_color = reflection * surface_properties.color();

                Color::DataType new_color_level = color_level * surface_color.max_element();
                if (new_color_level >= MIN_COLOR_LEVEL)
                {
                        Color direct = direct_diffuse_lighting(
                                ray_count, paint_data, point, geometric_normal, shading_normal, smooth_normal,
                                paint_data.ray_offset);

                        color += surface_color * direct;

                        Ray<N, T> new_ray;
                        if (diffuse_weighted_ray(
                                    paint_data, random_engine, point, shading_normal, geometric_normal, smooth_normal,
                                    &new_ray))
                        {
                                std::optional<Color> diffuse = trace_path(
                                        paint_data, ray_count, random_engine, recursion_level + 1, new_color_level,
                                        new_ray);

                                color += surface_color
                                         * diffuse.value_or(paint_data.scene.background_light_source_color());
                        }
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

template <typename VectorType, size_t N, typename ArrayType>
Vector<N, VectorType> array_to_vector(const std::array<ArrayType, N>& array)
{
        Vector<N, VectorType> res;
        for (unsigned i = 0; i < N; ++i)
        {
                res[i] = array[i];
        }
        return res;
}

template <size_t N, typename T>
void paint_pixels(
        unsigned thread_number,
        PainterRandomEngine<T>& random_engine,
        std::vector<Vector<N - 1, T>>* samples,
        std::atomic_bool* stop,
        const Projector<N, T>& projector,
        const PaintData<N, T>& paint_data,
        PainterNotifier<N - 1>* painter_notifier,
        Paintbrush<N - 1>* paintbrush,
        const PainterSampler<N - 1, T>& sampler,
        Pixels<N - 1>* pixels)
{
        std::optional<std::array<int_least16_t, N - 1>> pixel;

        Counter ray_count;
        int sample_count = 0;

        while (!(*stop) && (pixel = paintbrush->next_pixel(ray_count.value(), sample_count)))
        {
                painter_notifier->painter_pixel_before(thread_number, *pixel);

                Vector<N - 1, T> screen_point = array_to_vector<T>(*pixel);

                sampler.generate(random_engine, samples);

                sample_count = samples->size();
                int hit_sample_count = 0;

                ray_count.reset();
                Color color(0);

                for (const Vector<N - 1, T>& sample_point : *samples)
                {
                        constexpr int recursion_level = 0;
                        constexpr Color::DataType color_level = 1;

                        Ray<N, T> ray = projector.ray(screen_point + sample_point);

                        std::optional<Color> sample_color =
                                trace_path(paint_data, &ray_count, random_engine, recursion_level, color_level, ray);

                        if (sample_color)
                        {
                                color += *sample_color;
                                ++hit_sample_count;
                        }
                }

                Color pixel_color;
                float coverage;
                pixels->add_color_and_samples(*pixel, color, hit_sample_count, sample_count, &pixel_color, &coverage);

                painter_notifier->painter_pixel_after(thread_number, *pixel, pixel_color, coverage);
        }
}

template <size_t N, typename T>
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
                        RandomEngineWithSeed<PainterRandomEngine<T>> random_engine;

                        std::vector<Vector<N - 1, T>> samples;

                        while (true)
                        {
                                paint_pixels(
                                        thread_number, random_engine, &samples, stop, projector, paint_data,
                                        painter_notifier, paintbrush, sampler, pixels);

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

void check_thread_count(int thread_count)
{
        if (thread_count < 1)
        {
                error("Painter thread count (" + to_string(thread_count) + ") must be greater than 0");
        }
}

template <size_t N, typename T>
void check_paintbrush_projector(const Paintbrush<N - 1>& paintbrush, const Projector<N, T>& projector)
{
        if (paintbrush.screen_size() != projector.screen_size())
        {
                error("The paintbrush screen size (" + to_string(paintbrush.screen_size())
                      + ") are not equal to the projector screen size (" + to_string(projector.screen_size()) + ")");
        }
}

template <size_t N, typename T>
void paint_threads(
        PainterNotifier<N - 1>* painter_notifier,
        int samples_per_pixel,
        const Scene<N, T>& scene,
        Paintbrush<N - 1>* paintbrush,
        int thread_count,
        std::atomic_bool* stop,
        bool smooth_normal)

{
        check_thread_count(thread_count);
        check_paintbrush_projector(*paintbrush, scene.projector());

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
template <size_t N, typename T>
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
