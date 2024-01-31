/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/random/pcg.h>
#include <src/numerical/vector.h>
#include <src/painter/objects.h>
#include <src/painter/painter.h>
#include <src/painter/pixels/pixels.h>

#include <atomic>
#include <cstddef>
#include <optional>
#include <vector>

namespace ns::painter::painting
{
template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
class IntegratorPT final
{
        const Scene<N, T, Color>* const scene_;
        const Projector<N, T>* const projector_;
        const std::atomic_bool* const stop_;
        Statistics* const statistics_;
        Notifier<N - 1>* const notifier_;
        pixels::Pixels<N - 1, T, Color>* const pixels_;

        const SamplerStratifiedJittered<N - 1, T> sampler_;
        Paintbrush<N - 1> paintbrush_;

        [[nodiscard]] bool integrate(
                unsigned thread_number,
                PCG& engine,
                std::vector<numerical::Vector<N - 1, T>>& sample_points,
                std::vector<std::optional<Color>>& sample_colors);

public:
        IntegratorPT(
                const Scene<N, T, Color>* scene,
                const std::atomic_bool* stop,
                Statistics* statistics,
                Notifier<N - 1>* notifier,
                pixels::Pixels<N - 1, T, Color>* pixels,
                int samples_per_pixel);

        void next_pass();

        void integrate(unsigned thread_number);
};
}
