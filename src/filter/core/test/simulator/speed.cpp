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

#include "speed.h"

#include <src/com/random/pcg.h>
#include <src/filter/core/test/measurements.h>

#include <cmath>
#include <random>
#include <vector>

namespace ns::filter::core::test::simulator
{
namespace
{
template <typename T>
class SpeedSimulator final
{
        const T dt_;
        const T measurement_variance_x_;
        const T measurement_variance_v_;
        PCG engine_;
        std::normal_distribution<T> nd_process_v_;
        std::normal_distribution<T> nd_measurement_x_;
        std::normal_distribution<T> nd_measurement_v_;
        unsigned long long index_{0};
        T time_{0};
        T x_;
        T v_{nd_process_v_(engine_)};

public:
        SpeedSimulator(
                const T init_x,
                const T dt,
                const T process_velocity_mean,
                const T process_velocity_variance,
                const T measurement_variance_x,
                const T measurement_variance_v)
                : dt_(dt),
                  measurement_variance_x_(measurement_variance_x),
                  measurement_variance_v_(measurement_variance_v),
                  nd_process_v_(process_velocity_mean, std::sqrt(process_velocity_variance)),
                  nd_measurement_x_(0, std::sqrt(measurement_variance_x)),
                  nd_measurement_v_(0, std::sqrt(measurement_variance_v)),
                  x_(init_x)
        {
        }

        void move()
        {
                ++index_;
                time_ = index_ * dt_;
                const T v_next = nd_process_v_(engine_);
                const T v_average = (v_ + v_next) / 2;
                x_ += dt_ * v_average;
                v_ = v_next;
        }

        [[nodiscard]] T time() const
        {
                return time_;
        }

        [[nodiscard]] T x() const
        {
                return x_;
        }

        [[nodiscard]] T v() const
        {
                return v_;
        }

        [[nodiscard]] Measurement<T> measurement_x()
        {
                const T x = x_ + nd_measurement_x_(engine_);
                return {.value = x, .variance = measurement_variance_x_};
        }

        [[nodiscard]] Measurement<T> measurement_v()
        {
                const T v = v_ + nd_measurement_v_(engine_);
                return {.value = v, .variance = measurement_variance_v_};
        }
};

template <typename T>
std::vector<Measurements<T>> simulate(const T length, SpeedSimulator<T>* const simulator)
{
        std::vector<Measurements<T>> res;

        while (simulator->time() <= length)
        {
                res.push_back(
                        {.time = simulator->time(),
                         .true_x = simulator->x(),
                         .true_v = simulator->v(),
                         .x = simulator->measurement_x(),
                         .v = simulator->measurement_v()});

                simulator->move();
        }

        return res;
}
}

template <typename T>
std::vector<Measurements<T>> simulate_speed(
        const T length,
        const T init_x,
        const T dt,
        const T process_velocity_mean,
        const T process_velocity_variance,
        const T measurement_variance_x,
        const T measurement_variance_v)
{
        SpeedSimulator<T> simulator(
                init_x, dt, process_velocity_mean, process_velocity_variance, measurement_variance_x,
                measurement_variance_v);

        return simulate(length, &simulator);
}

#define INSTANTIATION(T) template std::vector<Measurements<T>> simulate_speed(T, T, T, T, T, T, T);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
