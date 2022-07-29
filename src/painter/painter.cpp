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

#include "painting/painting.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/thread.h>
#include <src/settings/instantiation.h>

#include <atomic>
#include <barrier>

namespace ns::painter
{
namespace
{
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
             const Scene<N, T, Color>* const scene,
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
                        [=, stop = &stop_, statistics = statistics_.get(), scene = scene]() noexcept
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
        const Scene<N, T, Color>* const scene,
        const int thread_count,
        const bool flat_shading)
{
        if (!flat_shading)
        {
                return std::make_unique<Impl>(
                        notifier, samples_per_pixel, max_pass_count, scene, thread_count, std::false_type());
        }
        return std::make_unique<Impl>(
                notifier, samples_per_pixel, max_pass_count, scene, thread_count, std::true_type());
}

#define TEMPLATE(N, T, C)                                 \
        template std::unique_ptr<Painter> create_painter( \
                Notifier<(N)-1>*, int, std::optional<int>, const Scene<(N), T, C>*, int, bool);

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
