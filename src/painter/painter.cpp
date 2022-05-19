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

#include <atomic>
#include <barrier>
#include <random>

namespace ns::painter
{
namespace
{
void join_thread(std::thread* const thread) noexcept
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

template <std::size_t N, typename T, typename Color>
struct PaintData final
{
        const Scene<N, T, Color>* scene;
        bool smooth_normals;

        PaintData(const Scene<N, T, Color>* const scene, const bool smooth_normals)
                : scene(scene),
                  smooth_normals(smooth_normals)
        {
                ASSERT(scene);
        }
};

template <std::size_t N, typename T, typename Color>
struct PixelData final
{
        const Projector<N, T>* projector;
        SamplerStratifiedJittered<N - 1, T> sampler;
        Pixels<N - 1, T, Color> pixels;

        PixelData(
                const Projector<N, T>* const projector,
                const int samples_per_pixel,
                const Color& background_color,
                Notifier<N - 1>* const notifier)
                : projector(projector),
                  sampler(samples_per_pixel),
                  pixels(projector->screen_size(), background_color, notifier)
        {
                ASSERT(projector);
        }
};

class PassData final
{
        std::optional<int> max_number_;
        int number_ = 0;

public:
        explicit PassData(const std::optional<int> max_number) : max_number_(max_number)
        {
                ASSERT(!max_number || *max_number > 0);
        }

        [[nodiscard]] bool continue_painting()
        {
                return !(max_number_ && ++number_ == *max_number_);
        }
};

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

template <std::size_t N, typename T, typename Color>
void paint_pixels(
        const unsigned thread_number,
        const PaintData<N, T, Color>& paint_data,
        std::atomic_bool* const stop,
        PixelData<N, T, Color>* const pixel_data,
        PaintingStatistics* const statistics,
        Notifier<N - 1>* const notifier)
{
        thread_local PCG engine;
        thread_local std::vector<Vector<N - 1, T>> sample_points;
        thread_local std::vector<std::optional<Color>> sample_colors;

        while (true)
        {
                MemoryArena::thread_local_instance().clear();

                if (*stop)
                {
                        return;
                }

                const std::optional<std::array<int, N - 1>> pixel = pixel_data->pixels.next_pixel();
                if (!pixel)
                {
                        return;
                }

                ThreadBusy thread_busy(notifier, thread_number, *pixel);

                const Vector<N - 1, T> pixel_org = to_vector<T>(*pixel);

                pixel_data->sampler.generate(engine, &sample_points);
                sample_colors.resize(sample_points.size());

                const long long ray_count = paint_data.scene->thread_ray_count();

                for (std::size_t i = 0; i < sample_points.size(); ++i)
                {
                        const Ray<N, T> ray = pixel_data->projector->ray(pixel_org + sample_points[i]);
                        sample_colors[i] =
                                trace_path<N, T, Color>(*paint_data.scene, paint_data.smooth_normals, ray, engine);
                }

                pixel_data->pixels.add_samples(*pixel, sample_points, sample_colors);
                statistics->pixel_done(paint_data.scene->thread_ray_count() - ray_count, sample_points.size());
        }
}

template <std::size_t N, typename T, typename Color>
void prepare_next_pass(
        const unsigned thread_number,
        std::atomic_bool* const stop,
        PixelData<N, T, Color>* const pixel_data,
        PassData* const pass_data,
        PaintingStatistics* const statistics,
        Notifier<N - 1>* const notifier)
{
        if (thread_number != 0)
        {
                return;
        }

        statistics->pass_done();

        const long long pass_number = statistics->statistics().pass_number;

        {
                ImagesWriting lock(notifier->images(pass_number));
                pixel_data->pixels.images(&lock.image_with_background(), &lock.image_without_background());
        }

        notifier->pass_done(pass_number);

        if (pass_data->continue_painting())
        {
                statistics->next_pass();
                pixel_data->pixels.next_pass();
                pixel_data->sampler.next_pass();
        }
        else
        {
                *stop = true;
        }
}

template <std::size_t N, typename T, typename Color>
void worker_thread(
        const unsigned thread_number,
        std::barrier<>* const barrier,
        const PaintData<N, T, Color>& paint_data,
        std::atomic_bool* const stop,
        PixelData<N, T, Color>* const pixel_data,
        PassData* const pass_data,
        PaintingStatistics* const statistics,
        Notifier<N - 1>* const notifier)
{
        while (true)
        {
                try
                {
                        paint_pixels(thread_number, paint_data, stop, pixel_data, statistics, notifier);
                }
                catch (const std::exception& e)
                {
                        *stop = true;
                        notifier->error_message(std::string("Painter error:\n") + e.what());
                }
                catch (...)
                {
                        *stop = true;
                        notifier->error_message("Unknown painter error");
                }

                barrier->arrive_and_wait();

                if (*stop)
                {
                        return;
                }

                try
                {
                        prepare_next_pass<N, T>(thread_number, stop, pixel_data, pass_data, statistics, notifier);
                }
                catch (const std::exception& e)
                {
                        *stop = true;
                        notifier->error_message(std::string("Painter error:\n") + e.what());
                }
                catch (...)
                {
                        *stop = true;
                        notifier->error_message("Unknown painter error");
                }

                barrier->arrive_and_wait();

                if (*stop)
                {
                        return;
                }
        }
}

template <std::size_t N, typename T, typename Color>
void worker_threads(
        const int thread_count,
        const PaintData<N, T, Color>& paint_data,
        std::atomic_bool* const stop,
        PixelData<N, T, Color>* const pixel_data,
        PassData* const pass_data,
        PaintingStatistics* const statistics,
        Notifier<N - 1>* const notifier)
{
        std::barrier barrier(thread_count);

        const auto f = [&](const unsigned thread_number) noexcept
        {
                try
                {
                        worker_thread(
                                thread_number, &barrier, paint_data, stop, pixel_data, pass_data, statistics, notifier);
                }
                catch (...)
                {
                        error_fatal("Exception in painter worker thread function");
                }
        };

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (int i = 0; i < thread_count; ++i)
        {
                threads.emplace_back(f, i);
        }

        for (std::thread& t : threads)
        {
                join_thread(&t);
        }
}

template <std::size_t N, typename T, typename Color>
void painter_thread(
        Notifier<N - 1>* const notifier,
        PaintingStatistics* const statistics,
        const int samples_per_pixel,
        const std::optional<int> max_pass_count,
        const Scene<N, T, Color>& scene,
        const int thread_count,
        std::atomic_bool* const stop,
        const bool smooth_normals) noexcept
{
        try
        {
                try
                {
                        const PaintData<N, T, Color> paint_data(&scene, smooth_normals);

                        PixelData<N, T, Color> pixel_data(
                                &scene.projector(), samples_per_pixel, scene.background_light(), notifier);

                        PassData pass_data(max_pass_count);

                        statistics->init();

                        worker_threads(thread_count, paint_data, stop, &pixel_data, &pass_data, statistics, notifier);
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
        template <std::size_t N, typename T, typename Color>
        Impl(Notifier<N - 1>* const notifier,
             const int samples_per_pixel,
             const std::optional<int> max_pass_count,
             std::shared_ptr<const Scene<N, T, Color>> scene,
             const int thread_count,
             const bool smooth_normals)
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
                        [=, stop = &stop_, statistics = statistics_.get(), scene = std::move(scene)]()
                        {
                                painter_thread(
                                        notifier, statistics, samples_per_pixel, max_pass_count, *scene, thread_count,
                                        stop, smooth_normals);
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
        const bool smooth_normals)
{
        return std::make_unique<Impl>(
                notifier, samples_per_pixel, max_pass_count, std::move(scene), thread_count, smooth_normals);
}

#define CREATE_PAINTER_INSTANTIATION_N_T_C(N, T, C)       \
        template std::unique_ptr<Painter> create_painter( \
                Notifier<(N)-1>*, int, std::optional<int>, std::shared_ptr<const Scene<(N), T, C>>, int, bool);

#define CREATE_PAINTER_INSTANTIATION_N_T(N, T)                   \
        CREATE_PAINTER_INSTANTIATION_N_T_C((N), T, color::Color) \
        CREATE_PAINTER_INSTANTIATION_N_T_C((N), T, color::Spectrum)

#define CREATE_PAINTER_INSTANTIATION_N(N)            \
        CREATE_PAINTER_INSTANTIATION_N_T((N), float) \
        CREATE_PAINTER_INSTANTIATION_N_T((N), double)

CREATE_PAINTER_INSTANTIATION_N(3)
CREATE_PAINTER_INSTANTIATION_N(4)
CREATE_PAINTER_INSTANTIATION_N(5)
CREATE_PAINTER_INSTANTIATION_N(6)
}
