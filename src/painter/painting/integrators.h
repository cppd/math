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

#pragma once

#include "paintbrush.h"
#include "sampler.h"
#include "statistics.h"
#include "thread_notifier.h"

#include "../integrators/pt/pt.h"
#include "../objects.h"
#include "../painter.h"
#include "../pixels/pixels.h"

#include <src/com/memory_arena.h>
#include <src/com/random/pcg.h>

#include <atomic>

namespace ns::painter
{
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
        static constexpr int PANTBRUSH_WIDTH = 20;

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
}
