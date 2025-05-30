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

#include "filter_0.h"

#include <src/com/error.h>
#include <src/filter/core/consistency.h>
#include <src/filter/core/update_info.h>
#include <src/filter/filters/measurement.h>

#include <cstddef>
#include <string>

namespace ns::filter::filters::position
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
};

template <std::size_t N, typename T>
void update_nees(const Filter0<N, T>& filter, const TrueData<N, T>& true_data, Nees<T>& nees)
{
        nees.position.add(true_data.position - filter.position(), filter.position_p());
}

template <typename Filter, std::size_t N, typename T>
void update_nees(const Filter& filter, const TrueData<N, T>& true_data, Nees<T>& nees)
{
        nees.position.add(true_data.position - filter.position(), filter.position_p());

        if (const T speed_p = filter.speed_p(); std::isfinite(speed_p))
        {
                nees.speed.add_1(true_data.speed - filter.speed(), speed_p);
        }
}

template <std::size_t N, typename T>
void update_nis(const core::UpdateInfo<N, T>& update, Nis<T>& nis)
{
        ASSERT(!update.gate);
        ASSERT(update.normalized_innovation_squared);
        nis.position.add_dof(*update.normalized_innovation_squared, N);
}

template <typename T>
std::string make_consistency_string(const Nees<T>& nees, const Nis<T>& nis);
}
