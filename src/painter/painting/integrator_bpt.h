/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../integrators/bpt/bpt.h"
#include "../objects.h"
#include "../painter.h"
#include "../pixels/pixels.h"

#include <src/com/memory_arena.h>
#include <src/com/random/pcg.h>

#include <atomic>

namespace ns::painter::painting
{
template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
class IntegratorBPT final
{
        static constexpr int PANTBRUSH_WIDTH = 20;
        static constexpr bool EQUAL_LIGHT_POWER = true;

        static T light_power(const LightSource<N, T, Color>& light_source)
        {
                if (EQUAL_LIGHT_POWER)
                {
                        return 1;
                }
                return light_source.power().luminance();
        }

        static std::vector<integrators::bpt::LightDistribution<N, T, Color>> light_distributions(
                const integrators::bpt::LightDistributionBase<N, T, Color>* const light_distribution_base,
                const unsigned thread_count)
        {
                std::vector<integrators::bpt::LightDistribution<N, T, Color>> light_distributions;
                light_distributions.reserve(thread_count);
                for (unsigned i = 0; i < thread_count; ++i)
                {
                        light_distributions.emplace_back(light_distribution_base);
                }
                return light_distributions;
        }

        const Scene<N, T, Color>* const scene_;
        const Projector<N, T>* const projector_;
        const std::atomic_bool* const stop_;
        Statistics* const statistics_;
        Notifier<N - 1>* const notifier_;
        pixels::Pixels<N - 1, T, Color>* const pixels_;

        const SamplerStratifiedJittered<N - 1, T> sampler_;
        Paintbrush<N - 1> paintbrush_;

        const integrators::bpt::LightDistributionBase<N, T, Color> light_distribution_base_;
        std::vector<integrators::bpt::LightDistribution<N, T, Color>> light_distributions_;

public:
        IntegratorBPT(
                const Scene<N, T, Color>* const scene,
                const std::atomic_bool* const stop,
                Statistics* const statistics,
                Notifier<N - 1>* const notifier,
                pixels::Pixels<N - 1, T, Color>* const pixels,
                const int samples_per_pixel,
                const unsigned thread_count)
                : scene_(scene),
                  projector_(&scene_->projector()),
                  stop_(stop),
                  statistics_(statistics),
                  notifier_(notifier),
                  pixels_(pixels),
                  sampler_(samples_per_pixel),
                  paintbrush_(projector_->screen_size(), PANTBRUSH_WIDTH),
                  light_distribution_base_(*scene_, light_power),
                  light_distributions_(light_distributions(&light_distribution_base_, thread_count))
        {
                ASSERT(scene_);
                ASSERT(stop_);
                ASSERT(statistics_);
                ASSERT(notifier_);
                ASSERT(pixels_);
        }

        IntegratorBPT(const IntegratorBPT&) = delete;
        IntegratorBPT(IntegratorBPT&&) = delete;
        IntegratorBPT& operator=(const IntegratorBPT&) = delete;
        IntegratorBPT& operator=(IntegratorBPT&&) = delete;

        void next_pass()
        {
                sampler_.next_pass();
                paintbrush_.next_pass();
        }

        void integrate(const unsigned thread_number)
        {
                thread_local PCG engine;
                thread_local std::vector<Vector<N - 1, T>> sample_points;
                thread_local std::vector<std::optional<Color>> sample_colors;

                ASSERT(thread_number < light_distributions_.size());
                integrators::bpt::LightDistribution<N, T, Color>& light_distribution =
                        light_distributions_[thread_number];

                while (true)
                {
                        MemoryArena::thread_local_instance().clear();

                        if (*stop_)
                        {
                                return;
                        }

                        const std::optional<std::array<int, N - 1>> pixel = paintbrush_.next_pixel();
                        if (!pixel)
                        {
                                return;
                        }

                        const ThreadNotifier thread_busy(notifier_, thread_number, *pixel);

                        const Vector<N - 1, T> pixel_org = to_vector<T>(*pixel);

                        sampler_.generate(engine, &sample_points);
                        sample_colors.resize(sample_points.size());

                        const long long ray_count = scene_->thread_ray_count();

                        for (std::size_t i = 0; i < sample_points.size(); ++i)
                        {
                                const Ray<N, T> ray = projector_->ray(pixel_org + sample_points[i]);

                                sample_colors[i] =
                                        integrators::bpt::bpt<FLAT_SHADING>(*scene_, ray, light_distribution, engine);
                        }

                        pixels_->add_samples(*pixel, sample_points, sample_colors);
                        statistics_->pixel_done(scene_->thread_ray_count() - ray_count, sample_points.size());
                }
        }
};
}
