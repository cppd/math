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

#include "consistency.h"

#include <src/com/error.h>
#include <src/filter/core/update_info.h>
#include <src/filter/filters/measurement.h>

#include <cstddef>
#include <optional>

namespace ns::filter::filters::speed
{
template <typename Filter, std::size_t N, typename T>
void update_position(
        Filter* const filter,
        const Measurement<N, T>& position,
        const std::optional<Measurement<1, T>>& speed,
        const std::optional<T> gate,
        const T dt,
        const T process_variance,
        const T fading_memory_alpha,
        Nis<T>& nis)
{
        filter->predict(dt, process_variance, fading_memory_alpha);

        if (speed)
        {
                const core::UpdateInfo<N + 1, T> update = filter->update_position_speed(position, *speed, gate);
                update_nis_position_speed(update, nis);
                update_nis(update, nis);
                return;
        }

        const core::UpdateInfo<N, T> update = filter->update_position(position, gate);
        update_nis_position(update, nis);
        update_nis(update, nis);
}

template <typename Filter, typename T>
void update_non_position(
        Filter* const filter,
        const std::optional<Measurement<1, T>>& speed,
        const std::optional<T> gate,
        const T dt,
        const T process_variance,
        const T fading_memory_alpha,
        Nis<T>& nis)
{
        filter->predict(dt, process_variance, fading_memory_alpha);

        if (speed)
        {
                const core::UpdateInfo<1, T> update = filter->update_speed(*speed, gate);
                update_nis(update, nis);
                return;
        }

        ASSERT(false);
}
}
