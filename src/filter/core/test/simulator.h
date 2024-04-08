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

#include <cstddef>
#include <vector>

namespace ns::filter::core::test
{
template <typename T>
struct Measurements final
{
        T time;
        T true_x;
        T true_v;
        T x;
        T x_variance;
        T v;
        T v_variance;
};

template <typename T>
std::vector<Measurements<T>> simulate(
        std::size_t count,
        T init_x,
        T dt,
        T process_velocity_mean,
        T process_velocity_variance,
        T measurement_variance_x,
        T measurement_variance_v);
}
