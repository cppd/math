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

#include <src/com/angle.h>
#include <src/com/error.h>
#include <src/filter/core/update_info.h>
#include <src/filter/filters/measurement.h>

#include <optional>

namespace ns::filter::filters::direction
{
template <typename Filter, typename Nees, typename T>
void update_nees(const Filter& filter, const TrueData<2, T>& true_data, std::optional<Nees>& nees)
{
        if (!nees)
        {
                nees.emplace();
        }

        nees->position.add(true_data.position - filter.position(), filter.position_p());
        nees->speed.add(true_data.speed - filter.speed(), filter.speed_p());
        nees->angle.add(normalize_angle(true_data.angle + true_data.angle_r - filter.angle()), filter.angle_p());
}

template <typename Filter, typename Nis, typename T>
void update_position(
        Filter* const filter,
        const Measurement<2, T>& position,
        const std::optional<Measurement<1, T>>& direction,
        const std::optional<Measurement<1, T>>& speed,
        const std::optional<T> gate,
        const T dt,
        std::optional<Nis>& nis)
{
        if (!nis)
        {
                nis.emplace();
        }

        const auto update_nis_head_position = [&](const auto& update)
        {
                if (!update.gate)
                {
                        ASSERT(update.s);
                        nis->position.add(update.residual.template head<2>(), update.s->template top_left<2, 2>());
                }
        };

        if (speed)
        {
                if (direction)
                {
                        filter->predict(dt);
                        const core::UpdateInfo<4, T> update =
                                filter->update_position_speed_direction(position, *speed, *direction, gate);
                        update_nis_head_position(update);
                        return;
                }

                filter->predict(dt);
                const core::UpdateInfo<3, T> update = filter->update_position_speed(position, *speed, gate);
                update_nis_head_position(update);
                return;
        }

        if (direction)
        {
                filter->predict(dt);
                const core::UpdateInfo<3, T> update = filter->update_position_direction(position, *direction, gate);
                update_nis_head_position(update);
                return;
        }

        filter->predict(dt);
        const core::UpdateInfo<2, T> update = filter->update_position(position, gate);
        update_nis_head_position(update);
}

template <typename Filter, typename Nis, typename T>
[[nodiscard]] bool update_non_position(
        Filter* const filter,
        const std::optional<Measurement<1, T>>& direction,
        const std::optional<Measurement<1, T>>& speed,
        const std::optional<T> gate,
        const T dt,
        std::optional<Nis>& /*nis*/)
{
        if (speed)
        {
                if (direction)
                {
                        filter->predict(dt);
                        filter->update_speed_direction(*speed, *direction, gate);
                        return true;
                }

                filter->predict(dt);
                filter->update_speed(*speed, gate);
                return true;
        }

        if (direction)
        {
                filter->predict(dt);
                filter->update_direction(*direction, gate);
                return true;
        }

        return false;
}
}
