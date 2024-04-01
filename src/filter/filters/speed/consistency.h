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
#include <src/filter/core/consistency.h>
#include <src/filter/core/update_info.h>
#include <src/filter/filters/measurement.h>

#include <cstddef>
#include <optional>
#include <string>

namespace ns::filter::filters::speed
{
template <typename T>
struct Nees final
{
        core::NormalizedSquared<T> position;
        core::NormalizedSquared<T> speed;
};

template <typename T>
struct Nis final
{
        core::NormalizedSquared<T> position;
        core::NormalizedSquared<T> position_speed;
        core::NormalizedSquared<T> nis;
};

template <typename Filter, std::size_t N, typename T>
void update_nees(const Filter& filter, const TrueData<N, T>& true_data, std::optional<Nees<T>>& nees)
{
        if (!nees)
        {
                nees.emplace();
        }
        nees->position.add(true_data.position - filter.position(), filter.position_p());
        nees->speed.add_1(true_data.speed - filter.speed(), filter.speed_p());
}

template <std::size_t N, typename T>
void update_nis_position_speed(const core::UpdateInfo<N, T>& update, Nis<T>& nis)
{
        static_assert(N >= 2);
        if (!update.gate)
        {
                ASSERT(update.s);
                nis.position.add(update.residual.template head<N - 1>(), update.s->template top_left<N - 1, N - 1>());
                nis.position_speed.add(update.residual, *update.s);
        }
}

template <std::size_t N, typename T>
void update_nis_position(const core::UpdateInfo<N, T>& update, Nis<T>& nis)
{
        if (!update.gate)
        {
                ASSERT(update.s);
                nis.position.add(update.residual, *update.s);
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
std::string make_consistency_string(const std::optional<Nees<T>>& nees, const std::optional<Nis<T>>& nis)
{
        std::string s;

        if (nees)
        {
                s += "NEES position; " + nees->position.check_string();
                s += '\n';
                s += "NEES speed; " + nees->speed.check_string();
        }

        if (nis)
        {
                if (!s.empty())
                {
                        s += '\n';
                }
                s += "NIS position; " + nis->position.check_string();
                s += '\n';
                s += "NIS position speed; " + nis->position_speed.check_string();
                s += '\n';
                s += "NIS; " + nis->nis.check_string();
        }

        return s;
}
}
