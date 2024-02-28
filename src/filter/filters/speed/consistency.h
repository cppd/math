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
template <std::size_t N, typename T>
struct Nees final
{
        core::NormalizedSquared<N, T> position;
        core::NormalizedSquared<1, T> speed;
};

template <std::size_t N, typename T>
struct Nis final
{
        core::NormalizedSquared<N, T> position;
        core::NormalizedSquared<N + 1, T> position_speed;
};

template <typename Filter, std::size_t N, typename T>
void update_nees(const Filter& filter, const TrueData<N, T>& true_data, std::optional<Nees<N, T>>& nees)
{
        if (!nees)
        {
                nees.emplace();
        }
        nees->position.add(true_data.position - filter.position(), filter.position_p());
        nees->speed.add(true_data.speed - filter.speed(), filter.speed_p());
}

template <std::size_t N, typename T>
void update_nis_position_speed(const core::UpdateInfo<N + 1, T>& update, Nis<N, T>& nis)
{
        if (!update.gate)
        {
                ASSERT(update.s);
                nis.position.add(update.residual.template head<N>(), update.s->template top_left<N, N>());
                nis.position_speed.add(update.residual, *update.s);
        }
}

template <std::size_t N, typename T>
void update_nis_position(const core::UpdateInfo<N, T>& update, Nis<N, T>& nis)
{
        if (!update.gate)
        {
                ASSERT(update.s);
                nis.position.add(update.residual.template head<N>(), update.s->template top_left<N, N>());
        }
}

template <std::size_t N, typename T>
std::string make_consistency_string(const std::optional<Nees<N, T>>& nees, const std::optional<Nis<N, T>>& nis)
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
        }

        return s;
}
}
