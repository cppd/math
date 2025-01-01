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

#pragma once

#include "statistics.h"

#include <src/painter/objects.h>
#include <src/painter/painter.h>

#include <atomic>
#include <cstddef>
#include <optional>

namespace ns::painter::painting
{
template <bool FLAT_SHADING, std::size_t N, typename T, typename Color>
void painting(
        Integrator integrator,
        Notifier<N - 1>* notifier,
        Statistics* statistics,
        int samples_per_pixel,
        std::optional<int> max_pass_count,
        const Scene<N, T, Color>& scene,
        int thread_count,
        std::atomic_bool* stop) noexcept;
}
