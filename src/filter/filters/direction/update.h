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
#include <src/filter/filters/noise_model.h>

#include <optional>

namespace ns::filter::filters::direction
{
template <typename Filter, typename T>
void update_position(
        Filter* const filter,
        const Measurement<2, T>& position,
        const std::optional<Measurement<1, T>>& direction,
        const std::optional<Measurement<1, T>>& speed,
        const std::optional<T> gate,
        const T dt,
        const NoiseModel<T>& position_noise_model,
        const NoiseModel<T>& angle_noise_model,
        const T fading_memory_alpha,
        Nis<T>& nis)
{
        filter->predict(dt, position_noise_model, angle_noise_model, fading_memory_alpha);

        if (speed)
        {
                if (direction)
                {
                        const core::UpdateInfo<4, T> update =
                                filter->update_position_speed_direction(position, *speed, *direction, gate);
                        update_nis_position_speed_direction(update, nis);
                        update_nis(update, nis);
                        return;
                }

                const core::UpdateInfo<3, T> update = filter->update_position_speed(position, *speed, gate);
                update_nis_position(update, nis);
                update_nis(update, nis);
                return;
        }

        if (direction)
        {
                const core::UpdateInfo<3, T> update = filter->update_position_direction(position, *direction, gate);
                update_nis_position(update, nis);
                update_nis(update, nis);
                return;
        }

        const core::UpdateInfo<2, T> update = filter->update_position(position, gate);
        update_nis_position(update, nis);
        update_nis(update, nis);
}

template <typename Filter, typename T>
void update_non_position(
        Filter* const filter,
        const std::optional<Measurement<1, T>>& direction,
        const std::optional<Measurement<1, T>>& speed,
        const std::optional<T> gate,
        const T dt,
        const NoiseModel<T>& position_noise_model,
        const NoiseModel<T>& angle_noise_model,
        const T fading_memory_alpha,
        Nis<T>& nis)
{
        filter->predict(dt, position_noise_model, angle_noise_model, fading_memory_alpha);

        if (speed)
        {
                if (direction)
                {
                        const core::UpdateInfo<2, T> update = filter->update_speed_direction(*speed, *direction, gate);
                        update_nis(update, nis);
                        return;
                }

                const core::UpdateInfo<1, T> update = filter->update_speed(*speed, gate);
                update_nis(update, nis);
                return;
        }

        if (direction)
        {
                const core::UpdateInfo<1, T> update = filter->update_direction(*direction, gate);
                update_nis(update, nis);
                return;
        }

        ASSERT(false);
}

template <typename T, typename Filter>
void update_velocity(
        Filter* const filter,
        const Measurement<2, T>& velocity,
        const std::optional<T> gate,
        const T dt,
        const NoiseModel<T>& position_noise_model,
        const NoiseModel<T>& angle_noise_model,
        const T fading_memory_alpha,
        Nis<T>& nis)
{
        filter->predict(dt, position_noise_model, angle_noise_model, fading_memory_alpha);

        const core::UpdateInfo<2, T> update = filter->update_velocity(velocity, gate);
        update_nis(update, nis);
}
}
