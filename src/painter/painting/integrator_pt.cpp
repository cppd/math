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

#include "integrator_pt.h"

#include "thread_notifier.h"

#include "../integrators/pt/pt.h"

#include <src/color/color.h>
#include <src/com/memory_arena.h>
#include <src/com/random/pcg.h>
#include <src/settings/instantiation.h>

namespace ns::painter::painting
{
namespace
{
constexpr int PANTBRUSH_WIDTH = 20;
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
IntegratorPT<FLAT_SHADING, N, T, Color>::IntegratorPT(
        const Scene<N, T, Color>* const scene,
        const std::atomic_bool* const stop,
        Statistics* const statistics,
        Notifier<N - 1>* const notifier,
        pixels::Pixels<N - 1, T, Color>* const pixels,
        const int samples_per_pixel)
        : scene_(scene),
          projector_(&scene_->projector()),
          stop_(stop),
          statistics_(statistics),
          notifier_(notifier),
          pixels_(pixels),
          sampler_(samples_per_pixel),
          paintbrush_(projector_->screen_size(), PANTBRUSH_WIDTH)
{
        ASSERT(scene_);
        ASSERT(stop_);
        ASSERT(statistics_);
        ASSERT(notifier_);
        ASSERT(pixels_);
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void IntegratorPT<FLAT_SHADING, N, T, Color>::next_pass()
{
        sampler_.next_pass();
        paintbrush_.next_pass();
}

template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void IntegratorPT<FLAT_SHADING, N, T, Color>::integrate(const unsigned thread_number)
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
                        sample_colors[i] = integrators::pt::pt<FLAT_SHADING>(*scene_, ray, engine);
                }

                pixels_->add_samples(*pixel, sample_points, sample_colors);
                statistics_->pixel_done(scene_->thread_ray_count() - ray_count, sample_points.size());
        }
}

#define TEMPLATE(N, T, C)                             \
        template class IntegratorPT<true, (N), T, C>; \
        template class IntegratorPT<false, (N), T, C>;

TEMPLATE_INSTANTIATION_N_T_C(TEMPLATE)
}
