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

#include <src/com/error.h>
#include <src/com/random/pcg.h>

#include <cmath>
#include <random>
#include <vector>

namespace ns::filter::core::test
{
template <typename T>
std::vector<Measurements<T>> simulate(
        const T length,
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

        const auto push = [&](const T time, const T x, const T v)
        {
                const T m_x = x + nd_measurement_x(engine);
                const T m_v = v + nd_measurement_v(engine);
                res.push_back({
                        .time = time,
                        .true_x = x,
                        .true_v = v,
                        .x = Measurement<T>{.value = m_x, .variance = measurement_variance_x},
                        .v = Measurement<T>{.value = m_v, .variance = measurement_variance_v}
                });
        };

        T x = init_x;
        T v = nd_process_v(engine);
        push(0, x, v);
        for (unsigned long long i = 1;; ++i)
        {
                const T time = i * dt;
                if (time >= length)
                {
                        break;
                }
                const T v_next = nd_process_v(engine);
                const T v_average = (v + v_next) / 2;
                x += dt * v_average;
                v = v_next;
                push(time, x, v);
        }

        return res;
}

template <typename T>
std::vector<Measurements<T>> simulate_acceleration(
        const T length,
        const T init_x,
        const T dt,
        const T process_acceleration,
        const T process_velocity_variance,
        const T measurement_variance_x,
        const T measurement_variance_v)
{
        PCG engine;

        std::normal_distribution<T> nd_process_v(0, std::sqrt(process_velocity_variance));
        std::normal_distribution<T> nd_measurement_x(0, std::sqrt(measurement_variance_x));
        std::normal_distribution<T> nd_measurement_v(0, std::sqrt(measurement_variance_v));

        std::vector<Measurements<T>> res;

        const auto push = [&](const T time, const T x, const T v)
        {
                const T m_x = x + nd_measurement_x(engine);
                const T m_v = v + (v > 0 ? nd_measurement_v(engine) : 0);
                res.push_back({
                        .time = time,
                        .true_x = x,
                        .true_v = v,
                        .x = Measurement<T>{.value = m_x, .variance = measurement_variance_x},
                        .v = Measurement<T>{.value = m_v, .variance = measurement_variance_v}
                });
        };

        const auto speed = [&](const T time)
        {
                constexpr T STANDING = 10;
                constexpr T ACCELERATION = 10;
                constexpr T UNIFORM = 60;

                const T p = std::fmod(time, STANDING + ACCELERATION + UNIFORM + ACCELERATION);
                if (p < STANDING)
                {
                        ASSERT(p >= 0);
                        return T{0};
                }
                if (p < STANDING + ACCELERATION)
                {
                        return process_acceleration * std::fmod(p, ACCELERATION) + nd_process_v(engine);
                }
                if (p < STANDING + ACCELERATION + UNIFORM)
                {
                        return process_acceleration * ACCELERATION + nd_process_v(engine);
                }
                ASSERT(p < STANDING + ACCELERATION + UNIFORM + ACCELERATION);
                return process_acceleration * ACCELERATION - process_acceleration * std::fmod(p, ACCELERATION)
                       + nd_process_v(engine);
        };

        T x = init_x;
        T v = 0;
        push(0, x, v);
        for (unsigned long long i = 1;; ++i)
        {
                const T time = i * dt;
                if (time >= length)
                {
                        break;
                }
                const T v_next = speed(time);
                const T v_average = (v + v_next) / 2;
                x += dt * v_average;
                v = v_next;
                push(time, x, v);
        }

        return res;
}

#define INSTANTIATION(T)                                                     \
        template std::vector<Measurements<T>> simulate(T, T, T, T, T, T, T); \
        template std::vector<Measurements<T>> simulate_acceleration(T, T, T, T, T, T, T);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
