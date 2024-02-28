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

#include <src/filter/core/update_info.h>
#include <src/filter/filters/measurement.h>

#include <optional>

namespace ns::filter::filters::acceleration
{
template <typename Filter, typename T>
void update_position(
        Filter* const filter,
        const Measurement<2, T>& position,
        const std::optional<Measurement<2, T>>& acceleration,
        const std::optional<Measurement<1, T>>& direction,
        const std::optional<Measurement<1, T>>& speed,
        const std::optional<T> gate,
        const T dt,
        std::optional<Nis<T>>& nis)
{
        if (!nis)
        {
                nis.emplace();
        }

        if (speed)
        {
                if (direction)
                {
                        if (acceleration)
                        {
                                filter->predict(dt);
                                const core::UpdateInfo<6, T> update =
                                        filter->update_position_speed_direction_acceleration(
                                                position, *speed, *direction, *acceleration, gate);
                                update_nis_position_speed_direction_acceleration(update, *nis);
                                return;
                        }

                        filter->predict(dt);
                        const core::UpdateInfo<4, T> update =
                                filter->update_position_speed_direction(position, *speed, *direction, gate);
                        update_nis_position(update, *nis);
                        return;
                }

                if (acceleration)
                {
                        filter->predict(dt);
                        const core::UpdateInfo<5, T> update =
                                filter->update_position_speed_acceleration(position, *speed, *acceleration, gate);
                        update_nis_position(update, *nis);
                        return;
                }

                filter->predict(dt);
                const core::UpdateInfo<3, T> update = filter->update_position_speed(position, *speed, gate);
                update_nis_position(update, *nis);
                return;
        }

        if (direction)
        {
                if (acceleration)
                {
                        filter->predict(dt);
                        const core::UpdateInfo<5, T> update = filter->update_position_direction_acceleration(
                                position, *direction, *acceleration, gate);
                        update_nis_position(update, *nis);
                        return;
                }

                filter->predict(dt);
                const core::UpdateInfo<3, T> update = filter->update_position_direction(position, *direction, gate);
                update_nis_position(update, *nis);
                return;
        }

        if (acceleration)
        {
                filter->predict(dt);
                const core::UpdateInfo<4, T> update =
                        filter->update_position_acceleration(position, *acceleration, gate);
                update_nis_position(update, *nis);
                return;
        }

        filter->predict(dt);
        const core::UpdateInfo<2, T> update = filter->update_position(position, gate);
        update_nis_position(update, *nis);
}

template <typename Filter, typename T>
[[nodiscard]] bool update_non_position(
        Filter* const filter,
        const std::optional<Measurement<2, T>>& acceleration,
        const std::optional<Measurement<1, T>>& direction,
        const std::optional<Measurement<1, T>>& speed,
        const std::optional<T> gate,
        const T dt,
        std::optional<Nis<T>>& /*nis*/)
{
        if (speed)
        {
                if (direction)
                {
                        if (acceleration)
                        {
                                filter->predict(dt);
                                filter->update_speed_direction_acceleration(*speed, *direction, *acceleration, gate);
                                return true;
                        }

                        filter->predict(dt);
                        filter->update_speed_direction(*speed, *direction, gate);
                        return true;
                }

                if (acceleration)
                {
                        filter->predict(dt);
                        filter->update_speed_acceleration(*speed, *acceleration, gate);
                        return true;
                }

                filter->predict(dt);
                filter->update_speed(*speed, gate);
                return true;
        }

        if (direction)
        {
                if (acceleration)
                {
                        filter->predict(dt);
                        filter->update_direction_acceleration(*direction, *acceleration, gate);
                        return true;
                }

                filter->predict(dt);
                filter->update_direction(*direction, gate);
                return true;
        }

        if (acceleration)
        {
                filter->predict(dt);
                filter->update_acceleration(*acceleration, gate);
                return true;
        }

        return false;
}
}
