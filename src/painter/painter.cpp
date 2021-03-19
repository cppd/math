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

#include "painter.h"

#include "painter/pixels.h"
#include "painter/sampler.h"
#include "painter/trace.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/thread.h>

#include <atomic>
#include <random>

namespace ns::painter
{
namespace
{
void join_thread(std::thread* thread) noexcept
{
        ASSERT(thread);
        try
        {
                try
                {
                        if (thread->joinable())
                        {
                                thread->join();
                        }
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("Error joining painter thread ") + e.what());
                }
                catch (...)
                {
                        error_fatal("Unknown error joining painter thread");
                }
        }
        catch (...)
        {
                error_fatal("Exception in painter join thread exception handlers");
        }
}

template <std::size_t N, typename T>
struct PixelData final
{
        const Projector<N, T>& projector;
        SamplerStratifiedJittered<N - 1, T> sampler;
        Pixels<N - 1, T> pixels;
        Paintbrush<N - 1>* const paintbrush;

        PixelData(const Projector<N, T>& projector, int samples_per_pixel, Paintbrush<N - 1>* paintbrush)
                : projector(projector),
                  sampler(samples_per_pixel),
                  pixels(projector.screen_size()),
                  paintbrush(paintbrush)
        {
                ASSERT(paintbrush);
                paintbrush->first_pass();
        }
};

template <std::size_t N, typename T>
void paint_pixels(
        unsigned thread_number,
        const PaintData<N, T>& paint_data,
        std::atomic_bool* stop,
        PixelData<N, T>* pixel_data,
        PainterNotifier<N - 1>* notifier)
{
        thread_local RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
        thread_local std::vector<Vector<N - 1, T>> samples;

        samples.clear();
        int ray_count = 0;

        while (true)
        {
                if (*stop)
                {
                        return;
                }

                std::optional<std::array<int_least16_t, N - 1>> pixel =
                        pixel_data->paintbrush->next_pixel(ray_count, samples.size());
                if (!pixel)
                {
                        return;
                }

                notifier->pixel_busy(thread_number, *pixel);

                pixel_data->sampler.generate(random_engine, &samples);
                ray_count = 0;
                Vector<N - 1, T> pixel_org = to_vector<T>(*pixel);

                for (const Vector<N - 1, T>& sample : samples)
                {
                        constexpr int RECURSION_LEVEL = 0;
                        constexpr Color::DataType COLOR_LEVEL = 1;

                        Ray<N, T> ray = pixel_data->projector.ray(pixel_org + sample);

                        std::optional<Color> color =
                                trace_path(paint_data, &ray_count, random_engine, RECURSION_LEVEL, COLOR_LEVEL, ray);

                        pixel_data->pixels.add_sample(*pixel, sample, color);
                }

                PixelInfo info = pixel_data->pixels.info(*pixel);
                notifier->pixel_set(thread_number, *pixel, info.color, info.coverage);
        }
}

template <std::size_t N, typename T>
void prepare_next_pass(unsigned thread_number, std::atomic_bool* stop, PixelData<N, T>* pixel_data)
{
        if (thread_number != 0)
        {
                return;
        }
        if (!pixel_data->paintbrush->next_pass())
        {
                *stop = true;
        }
        pixel_data->sampler.next_pass();
}

template <std::size_t N, typename T>
void worker_thread(
        unsigned thread_number,
        ThreadBarrier* barrier,
        const PaintData<N, T>& paint_data,
        std::atomic_bool* stop,
        PixelData<N, T>* pixel_data,
        PainterNotifier<N - 1>* notifier)
{
        while (true)
        {
                try
                {
                        paint_pixels(thread_number, paint_data, stop, pixel_data, notifier);
                        barrier->wait();
                }
                catch (const std::exception& e)
                {
                        *stop = true;
                        notifier->error_message(std::string("Painter error:\n") + e.what());
                        barrier->wait();
                }
                catch (...)
                {
                        *stop = true;
                        notifier->error_message("Unknown painter error");
                        barrier->wait();
                }

                if (*stop)
                {
                        return;
                }

                try
                {
                        prepare_next_pass<N, T>(thread_number, stop, pixel_data);
                        barrier->wait();
                }
                catch (const std::exception& e)
                {
                        *stop = true;
                        notifier->error_message(std::string("Painter error:\n") + e.what());
                        barrier->wait();
                }
                catch (...)
                {
                        *stop = true;
                        notifier->error_message("Unknown painter error");
                        barrier->wait();
                }

                if (*stop)
                {
                        return;
                }
        }
}

template <std::size_t N, typename T>
void worker_threads(
        int thread_count,
        const PaintData<N, T>& paint_data,
        std::atomic_bool* stop,
        PixelData<N, T>* pixel_data,
        PainterNotifier<N - 1>* notifier) noexcept
{
        try
        {
                ThreadBarrier barrier(thread_count);
                std::vector<std::thread> threads(thread_count);

                const auto f = [&](unsigned thread_number) noexcept
                {
                        try
                        {
                                worker_thread(thread_number, &barrier, paint_data, stop, pixel_data, notifier);
                        }
                        catch (...)
                        {
                                error_fatal("Exception in painter worker thread function");
                        }
                };

                for (unsigned i = 0; i < threads.size(); ++i)
                {
                        threads[i] = std::thread(f, i);
                }

                for (std::thread& t : threads)
                {
                        join_thread(&t);
                }
        }
        catch (...)
        {
                error_fatal("Exception in paint worker threads function");
        }
}

template <std::size_t N, typename T>
void painter_thread(
        PainterNotifier<N - 1>* notifier,
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
                        const PaintData paint_data(scene, smooth_normal);
                        PixelData<N, T> pixel_data(scene.projector(), samples_per_pixel, paintbrush);

                        worker_threads(thread_count, paint_data, stop, &pixel_data, notifier);
                }
                catch (const std::exception& e)
                {
                        notifier->error_message(std::string("Painter error:\n") + e.what());
                }
                catch (...)
                {
                        notifier->error_message("Unknown painter error");
                }
        }
        catch (...)
        {
                error_fatal("Exception in painter thread function");
        }
}

template <std::size_t N, typename T>
class Impl final : public Painter<N, T>
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        std::atomic_bool m_stop = false;
        std::thread m_thread;

        void wait() noexcept override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                join_thread(&m_thread);
        }

public:
        Impl(PainterNotifier<N - 1>* notifier,
             int samples_per_pixel,
             std::shared_ptr<const painter::Scene<N, T>> scene,
             Paintbrush<N - 1>* paintbrush,
             int thread_count,
             bool smooth_normal)
        {
                if (!notifier || !paintbrush)
                {
                        error("Painter parameters not specified");
                }

                if (thread_count < 1)
                {
                        error("Painter thread count (" + to_string(thread_count) + ") must be greater than 0");
                }

                if (paintbrush->screen_size() != scene->projector().screen_size())
                {
                        error("Painter paintbrush size (" + to_string(paintbrush->screen_size())
                              + ") are not equal to the projector size (" + to_string(scene->projector().screen_size())
                              + ")");
                }

                m_thread = std::thread(
                        [=, stop = &m_stop, scene = std::move(scene)]()
                        {
                                painter_thread(
                                        notifier, samples_per_pixel, *scene, paintbrush, thread_count, stop,
                                        smooth_normal);
                        });
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_stop = true;
                join_thread(&m_thread);
        }
};
}

template <std::size_t N, typename T>
std::unique_ptr<Painter<N, T>> create_painter(
        PainterNotifier<N - 1>* notifier,
        int samples_per_pixel,
        std::shared_ptr<const painter::Scene<N, T>> scene,
        Paintbrush<N - 1>* paintbrush,
        int thread_count,
        bool smooth_normal)
{
        return std::make_unique<Impl<N, T>>(
                notifier, samples_per_pixel, std::move(scene), paintbrush, thread_count, smooth_normal);
}

#define CREATE_PAINTER_INSTANTIATION(N, T)                                                                            \
        template std::unique_ptr<Painter<(N), T>> create_painter(                                                     \
                PainterNotifier<(N)-1>*, int, std::shared_ptr<const painter::Scene<(N), T>>, Paintbrush<(N)-1>*, int, \
                bool);

CREATE_PAINTER_INSTANTIATION(3, float)
CREATE_PAINTER_INSTANTIATION(4, float)
CREATE_PAINTER_INSTANTIATION(5, float)
CREATE_PAINTER_INSTANTIATION(6, float)

CREATE_PAINTER_INSTANTIATION(3, double)
CREATE_PAINTER_INSTANTIATION(4, double)
CREATE_PAINTER_INSTANTIATION(5, double)
CREATE_PAINTER_INSTANTIATION(6, double)
}
