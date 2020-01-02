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

#include "com/alg.h"
#include "com/color/color.h"
#include "com/error.h"
#include "com/global_index.h"
#include "com/random/engine.h"
#include "com/thread.h"
#include "com/type/limit.h"
#include "painter/coefficient/cosine_sphere.h"
#include "painter/sampling/sampler.h"
#include "painter/sampling/sphere.h"
#include "painter/space/ray_intersection.h"

#include <random>
#include <thread>
#include <type_traits>
#include <vector>

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

namespace
{
template <size_t N>
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

        const GlobalIndex<N, long long> m_global_index;
        std::vector<Pixel> m_pixels;

public:
        Pixels(const std::array<int, N>& screen_size) : m_global_index(screen_size)
        {
                m_pixels.resize(m_global_index.count());
        }

        Color add_color_and_samples(const std::array<int_least16_t, N>& pixel, const Color& color, int samples)
        {
                return m_pixels[m_global_index.compute(pixel)].add_color_and_samples(color, samples);
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

template <size_t N, typename T>
struct PaintData
{
        const std::vector<const GenericObject<N, T>*>& objects;
        const std::vector<const LightSource<N, T>*>& light_sources;
        const SurfaceProperties<N, T>& default_surface_properties;
        const T ray_offset;
        const bool smooth_normal;

        PaintData(const std::vector<const GenericObject<N, T>*>& objects_,
                  const std::vector<const LightSource<N, T>*>& light_sources_,
                  const SurfaceProperties<N, T>& default_surface_properties_, const T& ray_offset_, const bool smooth_normal_)
                : objects(objects_),
                  light_sources(light_sources_),
                  default_surface_properties(default_surface_properties_),
                  ray_offset(ray_offset_),
                  smooth_normal(smooth_normal_)
        {
        }
};

bool color_is_zero(const Color& c)
{
        return c.max_element() < MIN_COLOR_LEVEL;
}

// все объекты считаются непрозрачными
template <size_t N, typename T>
bool object_is_obstacle_to_light(const GenericObject<N, T>* object, const Ray<N, T>& ray, T distance_to_light_source)
{
        T distance_to_object;
        const Surface<N, T>* surface;
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

template <size_t N, typename T>
bool light_source_is_visible(Counter& ray_count, const std::vector<const GenericObject<N, T>*>& objects, const Ray<N, T>& ray,
                             T distance_to_light_source)
{
        for (const GenericObject<N, T>* object : objects)
        {
                ++ray_count;

                if (object_is_obstacle_to_light(object, ray, distance_to_light_source))
                {
                        return false;
                }
        }
        return true;
}

template <size_t N, typename T, typename Object>
bool ray_intersection_distance(const std::vector<const Object*>& objects, const Ray<N, T>& ray, T* intersection_distance)
{
        const Surface<N, T>* intersection_surface;
        const void* intersection_data;

        return ray_intersection(objects, ray, intersection_distance, &intersection_surface, &intersection_data);
}

template <size_t N, typename T>
Color direct_diffuse_lighting(Counter& ray_count, const std::vector<const GenericObject<N, T>*>& objects,
                              const std::vector<const LightSource<N, T>*> light_sources, const Vector<N, T>& p,
                              const Vector<N, T>& geometric_normal, const Vector<N, T>& shading_normal, bool mesh,
                              const T& ray_offset, bool smooth_normal)
{
        Color color(0);

        for (const LightSource<N, T>* light_source : light_sources)
        {
                Color light_source_color;
                Vector<N, T> vector_to_light;

                light_source->properties(p, &light_source_color, &vector_to_light);

                if (color_is_zero(light_source_color))
                {
                        continue;
                }

                Ray<N, T> ray_to_light = Ray<N, T>(p, vector_to_light);

                const T dot_light_and_normal = dot(ray_to_light.dir(), shading_normal);

                if (dot_light_and_normal <= DOT_PRODUCT_EPSILON<T>)
                {
                        // Свет находится по другую сторону поверхности
                        continue;
                }

                if (!mesh || !smooth_normal || dot(ray_to_light.dir(), geometric_normal) >= 0)
                {
                        // Если объект не состоит из симплексов или геометрическая сторона обращена
                        // к источнику света, то напрямую рассчитать видимость источника света.

                        ray_to_light.move_along_dir(ray_offset);
                        if (!light_source_is_visible(ray_count, objects, ray_to_light, length(vector_to_light)))
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

                        ++ray_count;
                        ray_to_light.move_along_dir(ray_offset);
                        T t;
                        if (!ray_intersection_distance(objects, ray_to_light, &t))
                        {
                                // Если луч к источнику света направлен внутрь поверхности, и нет повторного
                                // пересечения с поверхностью, то нет освещения в точке.
                                continue;
                        }

                        T distance_to_light_source = length(vector_to_light);

                        if (t >= distance_to_light_source)
                        {
                                // Источник света находится внутри поверхности
                                continue;
                        }

                        ++ray_count;
                        Ray<N, T> ray_from_light = ray_to_light.reverse_ray();
                        ray_from_light.move_along_dir(2 * ray_offset);
                        T t_reverse;
                        if (ray_intersection_distance(objects, ray_from_light, &t_reverse) && (t_reverse < t))
                        {
                                // Если для луча, направленного от поверхности и от источника света,
                                // имеется пересечение с поверхностью на расстоянии меньше, чем расстояние
                                // до пересечения внутрь поверхности к источнику света, то предполагается,
                                // что точка находится по другую сторону от источника света.
                                continue;
                        }

                        ray_to_light.move_along_dir(t + ray_offset);
                        if (!light_source_is_visible(ray_count, objects, ray_to_light, distance_to_light_source - t))
                        {
                                continue;
                        }
                }

                T light_weight = DIFFUSE_LIGHT_COEFFICIENT<N, T> * dot_light_and_normal;

                color += light_source_color * light_weight;
        }

        return color;
}

template <size_t N, typename T>
Color trace_path(const PaintData<N, T>& paint_data, Counter& ray_count, PainterRandomEngine<T>& random_engine,
                 int recursion_level, Color::DataType color_level, const Ray<N, T>& ray, bool diffuse_reflection);

template <size_t N, typename T>
Color diffuse_lighting(const PaintData<N, T>& paint_data, Counter& ray_count, PainterRandomEngine<T>& random_engine,
                       int recursion_level, Color::DataType color_level, const Vector<N, T>& point,
                       const Vector<N, T>& shading_normal, const Vector<N, T>& geometric_normal, bool mesh)
{
        if (recursion_level < MAX_RECURSION_LEVEL)
        {
                // Распределение случайного луча с вероятностью по косинусу угла между нормалью и случайным вектором.

                // Случайный вектор диффузного освещения надо определять от видимой нормали.
                Ray<N, T> diffuse_ray = Ray<N, T>(point, random_cosine_weighted_on_hemisphere(random_engine, shading_normal));

                if (mesh && dot(diffuse_ray.dir(), geometric_normal) <= DOT_PRODUCT_EPSILON<T>)
                {
                        // Если получившийся случайный вектор диффузного отражения показывает
                        // в другую сторону от поверхности, то диффузного освещения нет.
                        return Color(0);
                }

                diffuse_ray.move_along_dir(paint_data.ray_offset);

                return trace_path(paint_data, ray_count, random_engine, recursion_level + 1, color_level, diffuse_ray, true);
        }

        return Color(0);
}

template <size_t N, typename T>
Color trace_path(const PaintData<N, T>& paint_data, Counter& ray_count, PainterRandomEngine<T>& random_engine,
                 int recursion_level, Color::DataType color_level, const Ray<N, T>& ray, bool diffuse_reflection)
{
        ++ray_count;

        const Surface<N, T>* surface;
        T t;
        const void* intersection_data;

        if (!ray_intersection(paint_data.objects, ray, &t, &surface, &intersection_data))
        {
                return (paint_data.default_surface_properties.is_light_source() && diffuse_reflection) ?
                               paint_data.default_surface_properties.get_light_source_color() :
                               paint_data.default_surface_properties.get_color();
        }

        Vector<N, T> point = ray.point(t);

        const SurfaceProperties surface_properties = surface->properties(point, intersection_data);

        Vector<N, T> geometric_normal = surface_properties.get_geometric_normal();

        T dot_dir_and_geometric_normal = dot(ray.dir(), geometric_normal);

        bool mesh = surface_properties.is_mesh();

        if (std::abs(dot_dir_and_geometric_normal) <= DOT_PRODUCT_EPSILON<T>)
        {
                return Color(0);
        }

        if (surface_properties.is_light_source())
        {
                return (diffuse_reflection) ? surface_properties.get_light_source_color() : surface_properties.get_color();
        }

        Vector<N, T> shading_normal =
                (mesh && paint_data.smooth_normal) ? surface_properties.get_shading_normal() : geometric_normal;

        ASSERT(dot(geometric_normal, shading_normal) > DOT_PRODUCT_EPSILON<T>);

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

                Color::DataType new_color_level = color_level * surface_color.max_element();

                if (new_color_level >= MIN_COLOR_LEVEL)
                {
                        Color direct = direct_diffuse_lighting(ray_count, paint_data.objects, paint_data.light_sources, point,
                                                               geometric_normal, shading_normal, mesh, paint_data.ray_offset,
                                                               paint_data.smooth_normal);

                        Color diffuse = diffuse_lighting(paint_data, ray_count, random_engine, recursion_level, new_color_level,
                                                         point, shading_normal, geometric_normal, mesh);

                        color += surface_color * (direct + diffuse);
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
void paint_pixels(unsigned thread_number, PainterRandomEngine<T>& random_engine, std::vector<Vector<N - 1, T>>* samples,
                  std::atomic_bool& stop, const Projector<N, T>& projector, const PaintData<N, T>& paint_data,
                  PainterNotifier<N - 1>* painter_notifier, Paintbrush<N - 1>* paintbrush,
                  const PainterSampler<N - 1, T>& sampler, Pixels<N - 1>* pixels)
{
        std::array<int_least16_t, N - 1> pixel;

        Counter ray_count = 0, sample_count = 0;

        while (!stop && paintbrush->next_pixel(ray_count, sample_count, &pixel))
        {
                painter_notifier->painter_pixel_before(thread_number, pixel);

                Vector<N - 1, T> screen_point = array_to_vector<T>(pixel);

                sampler.generate(random_engine, samples);

                ray_count = 0;
                sample_count = samples->size();

                Color color(0);

                for (const Vector<N - 1, T>& sample_point : *samples)
                {
                        constexpr int recursion_level = 0;
                        constexpr Color::DataType color_level = 1;
                        constexpr bool diffuse_reflection = false;

                        Ray<N, T> ray = projector.ray(screen_point + sample_point);

                        color += trace_path(paint_data, ray_count, random_engine, recursion_level, color_level, ray,
                                            diffuse_reflection);
                }

                Color pixel_color = pixels->add_color_and_samples(pixel, color, samples->size());

                painter_notifier->painter_pixel_after(thread_number, pixel, pixel_color);
        }
}

template <size_t N, typename T>
void work_thread(unsigned thread_number, ThreadBarrier& barrier, std::atomic_bool& stop, std::atomic_bool& error_caught,
                 std::atomic_bool& stop_painting, const Projector<N, T>& projector, const PaintData<N, T>& paint_data,
                 PainterNotifier<N - 1>* painter_notifier, Paintbrush<N - 1>* paintbrush, const PainterSampler<N - 1, T>& sampler,
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
                                paint_pixels(thread_number, random_engine, &samples, stop, projector, paint_data,
                                             painter_notifier, paintbrush, sampler, pixels);

                                barrier.wait();

                                // Здесь может пройти только часть потоков из-за исключений в других
                                // потоках. Переменная error_caught поменяться не может в других потоках,
                                // так как она в них может меняться только до барьера.
                                if (error_caught)
                                {
                                        return;
                                }

                                // Здесь проходят все потоки

                                if (thread_number == 0)
                                {
                                        if (stop || !paintbrush->next_pass())
                                        {
                                                stop_painting = true;
                                        }
                                }

                                barrier.wait();

                                // Здесь проходят все потоки, а переменная stop_painting поменяться не может
                                // в других потоках, так как она в них может меняться только до барьера.

                                if (stop_painting)
                                {
                                        return;
                                }
                        }
                }
                catch (std::exception& e)
                {
                        stop = true;
                        error_caught = true;
                        painter_notifier->painter_error_message(std::string("Painter error:\n") + e.what());
                        barrier.wait();
                }
                catch (...)
                {
                        stop = true;
                        error_caught = true;
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

template <size_t N, typename T>
void check_paintbrush_projector(const Paintbrush<N - 1>& paintbrush, const Projector<N, T>& projector)
{
        if (paintbrush.screen_size() != projector.screen_size())
        {
                error("The paintbrush screen size (" + to_string(paintbrush.screen_size()) +
                      ") are not equal to the projector screen size (" + to_string(projector.screen_size()) + ")");
        }
}

template <size_t N, typename T>
T compute_ray_offset(const std::vector<const GenericObject<N, T>*>& objects)
{
        T all_max = limits<T>::lowest();

        for (const GenericObject<N, T>* object : objects)
        {
                Vector<N, T> min, max;
                object->min_max(&min, &max);

                for (unsigned i = 0; i < N; ++i)
                {
                        all_max = std::max(all_max, std::max(std::abs(min[i]), std::abs(max[i])));
                }
        }

        T dist = all_max * (RAY_OFFSET_IN_EPSILONS * limits<T>::epsilon());

        return dist;
}

template <size_t N, typename T>
void paint_threads(PainterNotifier<N - 1>* painter_notifier, int samples_per_pixel, const PaintObjects<N, T>& paint_objects,
                   Paintbrush<N - 1>* paintbrush, int thread_count, std::atomic_bool* stop, bool smooth_normal)

{
        check_thread_count(thread_count);
        check_paintbrush_projector(*paintbrush, paint_objects.projector());

        const PainterSampler<N - 1, T> sampler(samples_per_pixel);

        const PaintData paint_data(paint_objects.objects(), paint_objects.light_sources(),
                                   paint_objects.default_surface_properties(), compute_ray_offset(paint_objects.objects()),
                                   smooth_normal);

        Pixels pixels(paint_objects.projector().screen_size());

        ThreadBarrier barrier(thread_count);
        std::vector<std::thread> threads(thread_count);
        std::atomic_bool error_caught = false;
        std::atomic_bool stop_painting = false;

        paintbrush->first_pass();

        for (unsigned i = 0; i < threads.size(); ++i)
        {
                threads[i] = std::thread([&, i]() {
                        work_thread(i, barrier, *stop, error_caught, stop_painting, paint_objects.projector(), paint_data,
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
void paint(PainterNotifier<N - 1>* painter_notifier, int samples_per_pixel, const PaintObjects<N, T>& paint_objects,
           Paintbrush<N - 1>* paintbrush, int thread_count, std::atomic_bool* stop, bool smooth_normal) noexcept
{
        try
        {
                try
                {
                        ASSERT(painter_notifier && paintbrush && stop);

                        paint_threads(painter_notifier, samples_per_pixel, paint_objects, paintbrush, thread_count, stop,
                                      smooth_normal);
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

template void paint(PainterNotifier<2>* painter_notifier, int samples_per_pixel, const PaintObjects<3, float>& paint_objects,
                    Paintbrush<2>* paintbrush, int thread_count, std::atomic_bool* stop, bool smooth_normal) noexcept;
template void paint(PainterNotifier<3>* painter_notifier, int samples_per_pixel, const PaintObjects<4, float>& paint_objects,
                    Paintbrush<3>* paintbrush, int thread_count, std::atomic_bool* stop, bool smooth_normal) noexcept;
template void paint(PainterNotifier<4>* painter_notifier, int samples_per_pixel, const PaintObjects<5, float>& paint_objects,
                    Paintbrush<4>* paintbrush, int thread_count, std::atomic_bool* stop, bool smooth_normal) noexcept;
template void paint(PainterNotifier<5>* painter_notifier, int samples_per_pixel, const PaintObjects<6, float>& paint_objects,
                    Paintbrush<5>* paintbrush, int thread_count, std::atomic_bool* stop, bool smooth_normal) noexcept;

template void paint(PainterNotifier<2>* painter_notifier, int samples_per_pixel, const PaintObjects<3, double>& paint_objects,
                    Paintbrush<2>* paintbrush, int thread_count, std::atomic_bool* stop, bool smooth_normal) noexcept;
template void paint(PainterNotifier<3>* painter_notifier, int samples_per_pixel, const PaintObjects<4, double>& paint_objects,
                    Paintbrush<3>* paintbrush, int thread_count, std::atomic_bool* stop, bool smooth_normal) noexcept;
template void paint(PainterNotifier<4>* painter_notifier, int samples_per_pixel, const PaintObjects<5, double>& paint_objects,
                    Paintbrush<4>* paintbrush, int thread_count, std::atomic_bool* stop, bool smooth_normal) noexcept;
template void paint(PainterNotifier<5>* painter_notifier, int samples_per_pixel, const PaintObjects<6, double>& paint_objects,
                    Paintbrush<5>* paintbrush, int thread_count, std::atomic_bool* stop, bool smooth_normal) noexcept;
