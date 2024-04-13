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

#include "simulator.h"

#include "measurements.h"

#include <src/com/random/pcg.h>

#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

namespace ns::filter::core::test
{
template <typename T>
std::vector<Measurements<T>> simulate(
        const std::size_t count,
        const T init_x,
        const T dt,
        const T process_velocity_mean,
        const T process_velocity_variance,
        const T measurement_variance_x,
        const T measurement_variance_v)
{
        PCG engine;

        std::normal_distribution<T> nd_process_v(process_velocity_mean, std::sqrt(process_velocity_variance));
        std::normal_distribution<T> nd_measurement_x(0, std::sqrt(measurement_variance_x));
        std::normal_distribution<T> nd_measurement_v(0, std::sqrt(measurement_variance_v));

        std::vector<Measurements<T>> res;
        res.reserve(count);

        const auto push = [&](const T time, const T x, const T v)
        {
                const T m_x = x + nd_measurement_x(engine);
                const T m_v = v + nd_measurement_v(engine);
                res.push_back({
                        .time = time,
                        .true_x = x,
                        .true_v = v,
                        .x = {.value = m_x, .variance = measurement_variance_x},
                        .v = {.value = m_v, .variance = measurement_variance_v}
                });
        };

        T x = init_x;
        T v = nd_process_v(engine);
        push(0, x, v);
        for (std::size_t i = 1; i < count; ++i)
        {
                const T v_next = nd_process_v(engine);
                const T v_average = (v + v_next) / 2;
                x += dt * v_average;
                v = v_next;
                push(i, x, v);
        }

        return res;
}

#define INSTANTIATION(T) template std::vector<Measurements<T>> simulate(std::size_t, T, T, T, T, T, T);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
