/*
Copyright (C) 2017-2022 Topological Manifold

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
#include "painter/statistics.h"
#include "painter/trace.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/memory_arena.h>
#include <src/com/print.h>
#include <src/com/random/create.h>
#include <src/com/random/pcg.h>
#include <src/com/thread.h>
#include <src/com/type/limit.h>
#include <src/settings/instantiation.h>

#include <atomic>
#include <barrier>
#include <random>

namespace ns::painter
{
namespace
{
template <std::size_t N>
class ThreadBusy final
{
        Notifier<N>* notifier_;
        unsigned thread_;

public:
        ThreadBusy(Notifier<N>* const notifier, const unsigned thread, const std::array<int, N>& pixel)
                : notifier_(notifier),
                  thread_(thread)
        {
                notifier_->thread_busy(thread_, pixel);
        }

        ~ThreadBusy()
        {
                notifier_->thread_free(thread_);
        }

        ThreadBusy(const ThreadBusy&) = delete;
        ThreadBusy& operator=(const ThreadBusy&) = delete;
};

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
class Painting final
{
        const Scene<N, T, Color>* const scene_;
        const Projector<N, T>* const projector_;
        std::atomic_bool* const stop_;
        PaintingStatistics* const statistics_;
        Notifier<N - 1>* const notifier_;
        const SamplerStratifiedJittered<N - 1, T> sampler_;

        Pixels<N - 1, T, Color> pixels_;
        std::optional<int> pass_count_;

        void paint_pixels(unsigned thread_number);
        void prepare_next_pass(unsigned thread_number);
        [[nodiscard]] bool paint_pass(unsigned thread_number, std::barrier<>* barrier);

public:
        Painting(
                const Scene<N, T, Color>* scene,
                std::atomic_bool* stop,
                PaintingStatistics* statistics,
                Notifier<N - 1>* notifier,
                int samples_per_pixel,
                std::optional<int> max_pass_count);

        void paint(unsigned thread_number, std::barrier<>* barrier);
};

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
Painting<FLAT_SHADING, N, T, Color>::Painting(
        const Scene<N, T, Color>* const scene,
        std::atomic_bool* const stop,
        PaintingStatistics* const statistics,
        Notifier<N - 1>* const notifier,
        const int samples_per_pixel,
        const std::optional<int> max_pass_count)
        : scene_(scene),
          projector_(&scene->projector()),
          stop_(stop),
          statistics_(statistics),
          notifier_(notifier),
          sampler_(samples_per_pixel),
          pixels_(projector_->screen_size(), scene->background_light(), notifier),
          pass_count_(max_pass_count)
{
        ASSERT(scene_);
        ASSERT(projector_);
        ASSERT(stop_);
        ASSERT(statistics_);
        ASSERT(notifier_);
        ASSERT(!pass_count_ || *pass_count_ > 0);
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void Painting<FLAT_SHADING, N, T, Color>::paint_pixels(const unsigned thread_number)
{
        thread_local PCG engine;
        thread_local std::vector<Vector<N - 1, T>> sample_points;
        thread_local std::vector<std::optional<Color>> sample_colors;

        while (true)
        {
                MemoryArena::thread_local_instance().clear();

                if (*stop_)
                {
                        return;
                }

                const std::optional<std::array<int, N - 1>> pixel = pixels_.next_pixel();
                if (!pixel)
                {
                        return;
                }

                ThreadBusy thread_busy(notifier_, thread_number, *pixel);

                const Vector<N - 1, T> pixel_org = to_vector<T>(*pixel);

                sampler_.generate(engine, &sample_points);
                sample_colors.resize(sample_points.size());

                const long long ray_count = scene_->thread_ray_count();

                for (std::size_t i = 0; i < sample_points.size(); ++i)
                {
                        const Ray<N, T> ray = projector_->ray(pixel_org + sample_points[i]);
                        sample_colors[i] = trace_path<FLAT_SHADING>(*scene_, ray, engine);
                }

                pixels_.add_samples(*pixel, sample_points, sample_colors);
                statistics_->pixel_done(scene_->thread_ray_count() - ray_count, sample_points.size());
        }
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void Painting<FLAT_SHADING, N, T, Color>::prepare_next_pass(const unsigned thread_number)
{
        if (thread_number != 0)
        {
                return;
        }

        statistics_->pass_done();

        const long long pass_number = statistics_->statistics().pass_number;

        {
                ImagesWriting lock(notifier_->images(pass_number));
                pixels_.images(&lock.image_with_background(), &lock.image_without_background());
        }

        notifier_->pass_done(pass_number);

        if (!pass_count_ || --*pass_count_ > 0)
        {
                statistics_->next_pass();
                pixels_.next_pass();
                sampler_.next_pass();
        }
        else
        {
                *stop_ = true;
        }
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
bool Painting<FLAT_SHADING, N, T, Color>::paint_pass(const unsigned thread_number, std::barrier<>* const barrier)
{
        try
        {
                paint_pixels(thread_number);
        }
        catch (const std::exception& e)
        {
                *stop_ = true;
                notifier_->error_message(std::string("Painter error:\n") + e.what());
        }
        catch (...)
        {
                *stop_ = true;
                notifier_->error_message("Unknown painter error");
        }

        barrier->arrive_and_wait();

        if (*stop_)
        {
                return false;
        }

        try
        {
                prepare_next_pass(thread_number);
        }
        catch (const std::exception& e)
        {
                *stop_ = true;
                notifier_->error_message(std::string("Painter error:\n") + e.what());
        }
        catch (...)
        {
                *stop_ = true;
                notifier_->error_message("Unknown painter error");
        }

        barrier->arrive_and_wait();

        return !(*stop_);
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void Painting<FLAT_SHADING, N, T, Color>::paint(const unsigned thread_number, std::barrier<>* const barrier)
{
        try
        {
                while (paint_pass(thread_number, barrier))
                {
                }
        }
        catch (...)
        {
                error_fatal("Exception in painting function");
        }
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void worker_threads(const int thread_count, Painting<FLAT_SHADING, N, T, Color>& painting)
{
        std::barrier barrier(thread_count);

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (int i = 0; i < thread_count; ++i)
        {
                threads.emplace_back(
                        [&painting, &barrier, i]() noexcept
                        {
                                painting.paint(i, &barrier);
                        });
        }

        for (std::thread& t : threads)
        {
                join_thread(&t);
        }
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void painter_thread(
        Notifier<N - 1>* const notifier,
        PaintingStatistics* const statistics,
        const int samples_per_pixel,
        const std::optional<int> max_pass_count,
        const Scene<N, T, Color>& scene,
        const int thread_count,
        std::atomic_bool* const stop) noexcept
{
        try
        {
                try
                {
                        Painting<FLAT_SHADING, N, T, Color> painting(
                                &scene, stop, statistics, notifier, samples_per_pixel, max_pass_count);

                        statistics->init();

                        worker_threads(thread_count, painting);
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

class Impl final : public Painter
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        std::unique_ptr<PaintingStatistics> statistics_;

        std::atomic_bool stop_ = false;
        std::thread thread_;

        void wait() noexcept override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                join_thread(&thread_);
        }

        [[nodiscard]] Statistics statistics() const override
        {
                return statistics_->statistics();
        }

public:
        template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
        Impl(Notifier<N - 1>* const notifier,
             const int samples_per_pixel,
             const std::optional<int> max_pass_count,
             std::shared_ptr<const Scene<N, T, Color>> scene,
             const int thread_count,
             std::bool_constant<FLAT_SHADING>)
        {
                if (!notifier)
                {
                        error("Painter notifier is not specified");
                }

                if (!scene)
                {
                        error("Painter scene is not specified");
                }

                if (samples_per_pixel < 1)
                {
                        error("Painter samples per pixel (" + to_string(samples_per_pixel)
                              + ") must be greater than 0");
                }

                if (thread_count < 1)
                {
                        error("Painter thread count (" + to_string(thread_count) + ") must be greater than 0");
                }

                if (max_pass_count && *max_pass_count < 1)
                {
                        error("Painter maximum pass count (" + to_string(*max_pass_count) + ") must be greater than 0");
                }

                statistics_ =
                        std::make_unique<PaintingStatistics>(multiply_all<long long>(scene->projector().screen_size()));

                thread_ = std::thread(
                        [=, stop = &stop_, statistics = statistics_.get(), scene = std::move(scene)]() noexcept
                        {
                                painter_thread<FLAT_SHADING>(
                                        notifier, statistics, samples_per_pixel, max_pass_count, *scene, thread_count,
                                        stop);
                        });
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                stop_ = true;
                join_thread(&thread_);
        }
};
}

template <std::size_t N, typename T, typename Color>
std::unique_ptr<Painter> create_painter(
        Notifier<N - 1>* const notifier,
        const int samples_per_pixel,
        const std::optional<int> max_pass_count,
        std::shared_ptr<const Scene<N, T, Color>> scene,
        const int thread_count,
        const bool flat_shading)
{
        if (!flat_shading)
        {
                return std::make_unique<Impl>(
                        notifier, samples_per_pixel, max_pass_count, std::move(scene), thread_count, std::false_type());
        }
        return std::make_unique<Impl>(
                notifier, samples_per_pixel, max_pass_count, std::move(scene), thread_count, std::true_type());
}

#define TEMPLATE(N, T, C)                                 \
        template std::unique_ptr<Painter> create_painter( \
                Notifier<(N)-1>*, int, std::optional<int>, std::shared_ptr<const Scene<(N), T, C>>, int, bool);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
