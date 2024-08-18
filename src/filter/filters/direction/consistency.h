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
#include <src/filter/core/consistency.h>
#include <src/filter/core/update_info.h>
#include <src/filter/filters/measurement.h>

#include <cstddef>
#include <string>

namespace ns::filter::filters::direction
{
template <typename T>
struct Nees final
{
        core::NormalizedSquared<T> position;
        core::NormalizedSquared<T> speed;
        core::NormalizedSquared<T> angle;
};

template <typename T>
struct Nis final
{
        core::NormalizedSquared<T> position_speed_direction;
        core::NormalizedSquared<T> position;
        core::NormalizedSquared<T> nis;
};

template <typename Filter, typename T>
void update_nees(const Filter& filter, const TrueData<2, T>& true_data, Nees<T>& nees)
{
        nees.position.add(true_data.position - filter.position(), filter.position_p());
        nees.speed.add_1(true_data.speed - filter.speed(), filter.speed_p());
        nees.angle.add_1(normalize_angle(true_data.angle + true_data.angle_r - filter.angle()), filter.angle_p());
}

template <typename T>
void update_nis_position_speed_direction(const core::UpdateInfo<4, T>& update, Nis<T>& nis)
{
        if (!update.gate)
        {
                ASSERT(update.s);
                nis.position.add(update.residual.template head<2>(), update.s->template top_left<2, 2>());
                nis.position_speed_direction.add(update.residual, *update.s);
        }
}

template <std::size_t N, typename T>
void update_nis_position(const core::UpdateInfo<N, T>& update, Nis<T>& nis)
{
        static_assert(N >= 2);

        if (!update.gate)
        {
                ASSERT(update.s);
                nis.position.add(update.residual.template head<2>(), update.s->template top_left<2, 2>());
        }
}

template <std::size_t N, typename T>
void update_nis(const core::UpdateInfo<N, T>& update, Nis<T>& nis)
{
        static_assert(N >= 1);

        if (!update.gate)
        {
                ASSERT(update.normalized_innovation_squared);
                nis.nis.add_dof(*update.normalized_innovation_squared, N);
        }
}

template <typename T>
[[nodiscard]] std::string make_consistency_string(const Nees<T>& nees, const Nis<T>& nis);
}
