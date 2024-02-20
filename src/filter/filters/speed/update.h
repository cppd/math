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

#include <src/com/error.h>
#include <src/filter/core/update_info.h>
#include <src/filter/filters/measurement.h>

#include <cstddef>
#include <optional>

namespace ns::filter::filters::speed
{
template <typename Filter, typename Nees, std::size_t N, typename T>
void update_nees(const Filter& filter, const TrueData<N, T>& true_data, std::optional<Nees>& nees)
{
        if (!nees)
        {
                nees.emplace();
        }
        nees->position.add(true_data.position - filter.position(), filter.position_p());
        nees->speed.add(true_data.speed - filter.speed(), filter.speed_p());
}

template <typename Filter, typename Nis, std::size_t N, typename T>
void update_position(
        Filter* const filter,
        const Measurement<N, T>& position,
        const std::optional<Measurement<1, T>>& speed,
        const std::optional<T> gate,
        const T dt,
        std::optional<Nis>& nis)
{
        if (!nis)
        {
                nis.emplace();
        }

        const auto update_nis_position_speed = [&](const auto& update)
        {
                if (!update.gate)
                {
                        ASSERT(update.s);
                        nis->position.add(update.residual.template head<N>(), update.s->template top_left<N, N>());
                        nis->position_speed.add(update.residual, *update.s);
                }
        };

        const auto update_nis_position = [&](const auto& update)
        {
                if (!update.gate)
                {
                        ASSERT(update.s);
                        nis->position.add(update.residual.template head<N>(), update.s->template top_left<N, N>());
                }
        };

        if (speed)
        {
                filter->predict(dt);
                const core::UpdateInfo<N + 1, T> update = filter->update_position_speed(position, *speed, gate);
                update_nis_position_speed(update);
                return;
        }

        filter->predict(dt);
        const core::UpdateInfo<N, T> update = filter->update_position(position, gate);
        update_nis_position(update);
}

template <typename Filter, typename Nis, std::size_t N, typename T>
[[nodiscard]] bool update_non_position(
        Filter* const filter,
        const std::optional<Measurement<N, T>>& speed,
        const std::optional<T> gate,
        const T dt,
        std::optional<Nis>& /*nis*/)
{
        if (speed)
        {
                filter->predict(dt);
                filter->update_speed(*speed, gate);
                return true;
        }

        return false;
}
}
