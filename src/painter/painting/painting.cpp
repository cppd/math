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

#include "painting.h"

#include "paintbrush.h"
#include "sampler.h"
#include "thread_notifier.h"

#include "../integrators/pt/pt.h"
#include "../pixels/pixels.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/memory_arena.h>
#include <src/com/random/pcg.h>
#include <src/com/thread.h>
#include <src/settings/instantiation.h>

#include <array>
#include <barrier>
#include <vector>

namespace ns::painter
{
namespace
{
constexpr int PANTBRUSH_WIDTH = 20;

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
class Integrator
{
protected:
        ~Integrator() = default;

public:
        virtual void init(
                const Scene<N, T, Color>* scene,
                std::atomic_bool* stop,
                PaintingStatistics* statistics,
                Notifier<N - 1>* notifier,
                pixels::Pixels<N - 1, T, Color>* pixels,
                unsigned thread_count) = 0;

        virtual void integrate(unsigned thread_number) = 0;

        virtual void next_pass() = 0;
};

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
class IntegratorPT final : public Integrator<FLAT_SHADING, N, T, Color>
{
        const Scene<N, T, Color>* scene_ = nullptr;
        const Projector<N, T>* projector_ = nullptr;
        std::atomic_bool* stop_ = nullptr;
        PaintingStatistics* statistics_ = nullptr;
        Notifier<N - 1>* notifier_ = nullptr;
        pixels::Pixels<N - 1, T, Color>* pixels_ = nullptr;

        const SamplerStratifiedJittered<N - 1, T> sampler_;
        std::optional<Paintbrush<N - 1>> paintbrush_;

public:
        explicit IntegratorPT(const int samples_per_pixel)
                : sampler_(samples_per_pixel)
        {
        }

        void init(
                const Scene<N, T, Color>* const scene,
                std::atomic_bool* const stop,
                PaintingStatistics* const statistics,
                Notifier<N - 1>* const notifier,
                pixels::Pixels<N - 1, T, Color>* const pixels,
                const unsigned /*thread_count*/) override
        {
                ASSERT(scene);

                scene_ = scene;
                projector_ = &scene->projector();
                stop_ = stop;
                statistics_ = statistics;
                notifier_ = notifier;
                pixels_ = pixels;

                paintbrush_.emplace(projector_->screen_size(), PANTBRUSH_WIDTH);
        }

        void integrate(const unsigned thread_number) override
        {
                ASSERT(scene_);
                ASSERT(projector_);
                ASSERT(stop_);
                ASSERT(statistics_);
                ASSERT(notifier_);
                ASSERT(pixels_);

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

                        const std::optional<std::array<int, N - 1>> pixel = paintbrush_->next_pixel();
                        if (!pixel)
                        {
                                return;
                        }

                        ThreadNotifier thread_busy(notifier_, thread_number, *pixel);

                        const Vector<N - 1, T> pixel_org = to_vector<T>(*pixel);

                        sampler_.generate(engine, &sample_points);
                        sample_colors.resize(sample_points.size());

                        const long long ray_count = scene_->thread_ray_count();

                        for (std::size_t i = 0; i < sample_points.size(); ++i)
                        {
                                const Ray<N, T> ray = projector_->ray(pixel_org + sample_points[i]);
                                sample_colors[i] = integrators::pt::pt<FLAT_SHADING>(*scene_, ray, engine);
                        }

                        pixels_->add_samples(*pixel, sample_points, sample_colors);
                        statistics_->pixel_done(scene_->thread_ray_count() - ray_count, sample_points.size());
                }
        }

        void next_pass() override
        {
                ASSERT(paintbrush_);

                paintbrush_->next_pass();
                sampler_.next_pass();
        }
};

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
class Painting final
{
        std::atomic_bool* const stop_;
        PaintingStatistics* const statistics_;
        Notifier<N - 1>* const notifier_;
        pixels::Pixels<N - 1, T, Color>* const pixels_;
        Integrator<FLAT_SHADING, N, T, Color>* const integrator_;

        std::optional<int> pass_count_;
        std::atomic_int call_counter_ = 0;

        void prepare_next_pass(unsigned thread_number);
        [[nodiscard]] bool paint_pass(unsigned thread_number, std::barrier<>* barrier);
        void paint(unsigned thread_number, std::barrier<>* barrier) noexcept;

public:
        Painting(
                std::atomic_bool* const stop,
                PaintingStatistics* const statistics,
                Notifier<N - 1>* const notifier,
                pixels::Pixels<N - 1, T, Color>* const pixels,
                Integrator<FLAT_SHADING, N, T, Color>* const integrator,
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

        void paint(const Scene<N, T, Color>* scene, unsigned thread_count);
};

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

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
bool Painting<FLAT_SHADING, N, T, Color>::paint_pass(const unsigned thread_number, std::barrier<>* const barrier)
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

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void Painting<FLAT_SHADING, N, T, Color>::paint(const unsigned thread_number, std::barrier<>* const barrier) noexcept
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
void Painting<FLAT_SHADING, N, T, Color>::paint(const Scene<N, T, Color>* const scene, const unsigned thread_count)
{
        ASSERT(++call_counter_ == 1);

        statistics_->init();

        integrator_->init(scene, stop_, statistics_, notifier_, pixels_, thread_count);

        std::barrier barrier(thread_count);

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (unsigned i = 0; i < thread_count; ++i)
        {
                threads.emplace_back(
                        [this, &barrier, i]() noexcept
                        {
                                paint(i, &barrier);
                        });
        }

        for (std::thread& t : threads)
        {
                join_thread(&t);
        }
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void painting_impl(
        Notifier<N - 1>* const notifier,
        PaintingStatistics* const statistics,
        const int samples_per_pixel,
        const std::optional<int> max_pass_count,
        const Scene<N, T, Color>& scene,
        const int thread_count,
        std::atomic_bool* const stop)
{
        IntegratorPT<FLAT_SHADING, N, T, Color> integrator(samples_per_pixel);

        pixels::Pixels<N - 1, T, Color> pixels(scene.projector().screen_size(), scene.background_light(), notifier);

        Painting<FLAT_SHADING, N, T, Color> painting(stop, statistics, notifier, &pixels, &integrator, max_pass_count);

        painting.paint(&scene, thread_count);
}
}

template <std::size_t N, typename T, typename Color>
void painting(
        const bool flat_shading,
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
                        if (flat_shading)
                        {
                                painting_impl<true>(
                                        notifier, statistics, samples_per_pixel, max_pass_count, scene, thread_count,
                                        stop);
                        }
                        else
                        {
                                painting_impl<false>(
                                        notifier, statistics, samples_per_pixel, max_pass_count, scene, thread_count,
                                        stop);
                        }
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
        template void painting(                                                                                     \
                bool, Notifier<(N)-1>*, PaintingStatistics*, int, std::optional<int>, const Scene<(N), T, C>&, int, \
                std::atomic_bool*) noexcept;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
