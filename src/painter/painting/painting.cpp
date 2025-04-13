/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "painting.h"

#include "integrator_bpt.h"
#include "integrator_pt.h"
#include "statistics.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/thread.h>
#include <src/painter/objects.h>
#include <src/painter/painter.h>
#include <src/painter/pixels/pixels.h>
#include <src/settings/instantiation.h>

#include <atomic>
#include <barrier>
#include <cstddef>
#include <exception>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace ns::painter::painting
{
namespace
{
template <std::size_t N, typename T, typename Color, typename Integrator>
class Painting final
{
        std::atomic_bool* const stop_;
        Statistics* const statistics_;
        Notifier<N>* const notifier_;
        pixels::Pixels<N, T, Color>* const pixels_;
        Integrator* const integrator_;

        std::optional<int> pass_count_;
        std::atomic_int call_counter_ = 0;

        void prepare_next_pass(unsigned thread_number);

        [[nodiscard]] bool paint_pass(unsigned thread_number, std::barrier<>* barrier);

        void paint(unsigned thread_number, std::barrier<>* barrier) noexcept;

public:
        Painting(
                std::atomic_bool* const stop,
                Statistics* const statistics,
                Notifier<N>* const notifier,
                pixels::Pixels<N, T, Color>* const pixels,
                Integrator* const integrator,
                const std::optional<int> max_pass_count)
                : stop_(stop),
                  statistics_(statistics),
                  notifier_(notifier),
                  pixels_(pixels),
                  integrator_(integrator),
                  pass_count_(max_pass_count)
        {
                ASSERT(stop_);
                ASSERT(statistics_);
                ASSERT(notifier_);
                ASSERT(pixels_);
                ASSERT(integrator_);

                ASSERT(!pass_count_ || *pass_count_ > 0);
        }

        void paint(unsigned thread_count);
};

template <std::size_t N, typename T, typename Color, typename Integrator>
void Painting<N, T, Color, Integrator>::prepare_next_pass(const unsigned thread_number)
{
        if (thread_number != 0)
        {
                return;
        }

        statistics_->pass_done();

        const long long pass_number = statistics_->statistics().pass_number;

        {
                const ImagesWriting lock(notifier_->images(pass_number));
                pixels_->images(&lock.image_with_background(), &lock.image_without_background());
        }

        notifier_->pass_done(pass_number);

        if (!pass_count_ || --*pass_count_ > 0)
        {
                statistics_->next_pass();
                integrator_->next_pass();
        }
        else
        {
                *stop_ = true;
        }
}

template <std::size_t N, typename T, typename Color, typename Integrator>
bool Painting<N, T, Color, Integrator>::paint_pass(const unsigned thread_number, std::barrier<>* const barrier)
{
        try
        {
                integrator_->integrate(thread_number);
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

template <std::size_t N, typename T, typename Color, typename Integrator>
void Painting<N, T, Color, Integrator>::paint(const unsigned thread_number, std::barrier<>* const barrier) noexcept
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

template <std::size_t N, typename T, typename Color, typename Integrator>
void Painting<N, T, Color, Integrator>::paint(const unsigned thread_count)
{
        ASSERT(++call_counter_ == 1);

        statistics_->init();

        std::barrier barrier(thread_count);

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (unsigned i = 0; i < thread_count; ++i)
        {
                threads.emplace_back(
                        [this, &barrier, i] noexcept
                        {
                                paint(i, &barrier);
                        });
        }

        for (std::thread& t : threads)
        {
                join_thread(&t);
        }
}

template <std::size_t N, typename T, typename Color, typename Integrator>
void painting_impl(
        std::atomic_bool* const stop,
        Statistics* const statistics,
        Notifier<N>* const notifier,
        pixels::Pixels<N, T, Color>* const pixels,
        Integrator* const integrator,
        const std::optional<int> max_pass_count,
        const int thread_count)
{
        Painting painting(stop, statistics, notifier, pixels, integrator, max_pass_count);

        painting.paint(thread_count);
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void painting_impl(
        const Integrator integrator,
        Notifier<N - 1>* const notifier,
        Statistics* const statistics,
        const int samples_per_pixel,
        const std::optional<int> max_pass_count,
        const Scene<N, T, Color>& scene,
        const int thread_count,
        std::atomic_bool* const stop)
{
        pixels::Pixels<N - 1, T, Color> pixels(scene.projector().screen_size(), scene.background_color(), notifier);

        switch (integrator)
        {
        case Integrator::BPT:
        {
                IntegratorBPT<FLAT_SHADING, N, T, Color> integrator_bpt(
                        &scene, stop, statistics, notifier, &pixels, samples_per_pixel, thread_count);
                painting_impl(stop, statistics, notifier, &pixels, &integrator_bpt, max_pass_count, thread_count);
                return;
        }
        case Integrator::PT:
        {
                IntegratorPT<FLAT_SHADING, N, T, Color> integrator_pt(
                        &scene, stop, statistics, notifier, &pixels, samples_per_pixel);
                painting_impl(stop, statistics, notifier, &pixels, &integrator_pt, max_pass_count, thread_count);
                return;
        }
        }
        error("Unknown integrator " + to_string(enum_to_int(integrator)));
}
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void painting(
        const Integrator integrator,
        Notifier<N - 1>* const notifier,
        Statistics* const statistics,
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
                        painting_impl<FLAT_SHADING>(
                                integrator, notifier, statistics, samples_per_pixel, max_pass_count, scene,
                                thread_count, stop);
                }
                catch (const std::exception& e)
                {
                        notifier->error_message(std::string("Painting error:\n") + e.what());
                }
                catch (...)
                {
                        notifier->error_message("Unknown painting error");
                }
        }
        catch (...)
        {
                error_fatal("Exception in painting exception handlers");
        }
}

#define TEMPLATE(N, T, C)                                                                                           \
        template void painting<true, (N), T, C>(                                                                    \
                Integrator, Notifier<(N) - 1>*, Statistics*, int, std::optional<int>, const Scene<(N), T, C>&, int, \
                std::atomic_bool*) noexcept;                                                                        \
        template void painting<false, (N), T, C>(                                                                   \
                Integrator, Notifier<(N) - 1>*, Statistics*, int, std::optional<int>, const Scene<(N), T, C>&, int, \
                std::atomic_bool*) noexcept;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
