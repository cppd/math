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
#include <src/com/time.h>

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
                paintbrush->init();
        }
};

class PaintingStatistics
{
        static_assert(std::atomic<long long>::is_always_lock_free);

        std::atomic<long long> m_pixel_count;
        std::atomic<long long> m_ray_count;
        std::atomic<long long> m_sample_count;

        long long m_pass_number;
        TimePoint m_pass_start_time;
        long long m_pass_start_pixel_count;
        double m_previous_pass_duration;

        mutable SpinLock m_lock;

public:
        void init()
        {
                m_pixel_count = 0;
                m_ray_count = 0;
                m_sample_count = 0;

                m_pass_number = 1;
                m_pass_start_time = time();
                m_pass_start_pixel_count = 0;
                m_previous_pass_duration = 0;
        }

        void pixel_done(int ray_count, int sample_count)
        {
                m_pixel_count.fetch_add(1, std::memory_order_relaxed);
                m_ray_count.fetch_add(ray_count, std::memory_order_relaxed);
                m_sample_count.fetch_add(sample_count, std::memory_order_relaxed);
        }

        void pass_done(bool prepare_next)
        {
                const TimePoint now = time();
                const double previous_pass_duration = duration(m_pass_start_time, now);

                std::lock_guard lg(m_lock);

                m_previous_pass_duration = previous_pass_duration;
                if (prepare_next)
                {
                        ++m_pass_number;
                        m_pass_start_time = now;
                        m_pass_start_pixel_count = m_pixel_count;
                }
        }

        Statistics statistics() const
        {
                Statistics s;

                std::lock_guard lg(m_lock);

                s.pass_number = m_pass_number;
                s.pass_pixel_count = m_pixel_count - m_pass_start_pixel_count;
                s.previous_pass_duration = m_previous_pass_duration;

                s.pixel_count = m_pixel_count;
                s.ray_count = m_ray_count;
                s.sample_count = m_sample_count;

                return s;
        }
};

template <std::size_t N, typename T>
void paint_pixels(
        unsigned thread_number,
        const PaintData<N, T>& paint_data,
        std::atomic_bool* stop,
        PixelData<N, T>* pixel_data,
        PaintingStatistics* statistics,
        Notifier<N - 1>* notifier)
{
        thread_local RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
        thread_local std::vector<Vector<N - 1, T>> samples;

        while (true)
        {
                if (*stop)
                {
                        return;
                }

                std::optional<std::array<int_least16_t, N - 1>> pixel = pixel_data->paintbrush->next_pixel();
                if (!pixel)
                {
                        return;
                }

                notifier->pixel_busy(thread_number, *pixel);

                pixel_data->sampler.generate(random_engine, &samples);
                int ray_count = 0;
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

                statistics->pixel_done(ray_count, samples.size());
        }
}

template <std::size_t N, typename T>
void prepare_next_pass(
        unsigned thread_number,
        std::atomic_bool* stop,
        PixelData<N, T>* pixel_data,
        PaintingStatistics* statistics)
{
        if (thread_number != 0)
        {
                return;
        }
        if (pixel_data->paintbrush->next_pass())
        {
                statistics->pass_done(true);
                pixel_data->sampler.next_pass();
        }
        else
        {
                statistics->pass_done(false);
                *stop = true;
        }
}

template <std::size_t N, typename T>
void worker_thread(
        unsigned thread_number,
        ThreadBarrier* barrier,
        const PaintData<N, T>& paint_data,
        std::atomic_bool* stop,
        PixelData<N, T>* pixel_data,
        PaintingStatistics* statistics,
        Notifier<N - 1>* notifier)
{
        while (true)
        {
                try
                {
                        paint_pixels(thread_number, paint_data, stop, pixel_data, statistics, notifier);
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
                        prepare_next_pass<N, T>(thread_number, stop, pixel_data, statistics);
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
        PaintingStatistics* statistics,
        Notifier<N - 1>* notifier) noexcept
{
        try
        {
                ThreadBarrier barrier(thread_count);
                std::vector<std::thread> threads(thread_count);

                const auto f = [&](unsigned thread_number) noexcept
                {
                        try
                        {
                                worker_thread(
                                        thread_number, &barrier, paint_data, stop, pixel_data, statistics, notifier);
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
        Notifier<N - 1>* notifier,
        PaintingStatistics* statistics,
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

                        statistics->init();

                        worker_threads(thread_count, paint_data, stop, &pixel_data, statistics, notifier);
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

        PaintingStatistics m_statistics;

        void wait() noexcept override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                join_thread(&m_thread);
        }

        Statistics statistics() const override
        {
                return m_statistics.statistics();
        }

public:
        Impl(Notifier<N - 1>* notifier,
             int samples_per_pixel,
             std::shared_ptr<const Scene<N, T>> scene,
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
                        [=, stop = &m_stop, statistics = &m_statistics, scene = std::move(scene)]()
                        {
                                painter_thread(
                                        notifier, statistics, samples_per_pixel, *scene, paintbrush, thread_count, stop,
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
        Notifier<N - 1>* notifier,
        int samples_per_pixel,
        std::shared_ptr<const Scene<N, T>> scene,
        Paintbrush<N - 1>* paintbrush,
        int thread_count,
        bool smooth_normal)
{
        return std::make_unique<Impl<N, T>>(
                notifier, samples_per_pixel, std::move(scene), paintbrush, thread_count, smooth_normal);
}

#define CREATE_PAINTER_INSTANTIATION(N, T)                        \
        template std::unique_ptr<Painter<(N), T>> create_painter( \
                Notifier<(N)-1>*, int, std::shared_ptr<const Scene<(N), T>>, Paintbrush<(N)-1>*, int, bool);

CREATE_PAINTER_INSTANTIATION(3, float)
CREATE_PAINTER_INSTANTIATION(4, float)
CREATE_PAINTER_INSTANTIATION(5, float)
CREATE_PAINTER_INSTANTIATION(6, float)

CREATE_PAINTER_INSTANTIATION(3, double)
CREATE_PAINTER_INSTANTIATION(4, double)
CREATE_PAINTER_INSTANTIATION(5, double)
CREATE_PAINTER_INSTANTIATION(6, double)
}
