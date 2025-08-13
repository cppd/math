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

#include "acceleration.h"

#include <src/com/error.h>
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
class AccelerationSimulator final
{
        const T dt_;
        const T process_acceleration_;
        const T measurement_variance_x_;
        const T measurement_variance_v_;
        PCG engine_;
        std::normal_distribution<T> nd_process_v_;
        std::normal_distribution<T> nd_measurement_x_;
        std::normal_distribution<T> nd_measurement_v_;
        unsigned long long index_{0};
        T time_{0};
        T x_;
        T v_{0};

        T speed()
        {
                constexpr T STANDING = 10;
                constexpr T ACCELERATION = 10;
                constexpr T UNIFORM = 65;
                constexpr T DECELERATION = 5;

                const T p = std::fmod(time_, STANDING + ACCELERATION + UNIFORM + DECELERATION);

                if (p < STANDING)
                {
                        ASSERT(p >= 0);
                        return T{0};
                }

                if (p < STANDING + ACCELERATION)
                {
                        const T t = p - STANDING;
                        return process_acceleration_ * t + nd_process_v_(engine_);
                }

                if (p < STANDING + ACCELERATION + UNIFORM)
                {
                        return process_acceleration_ * ACCELERATION + nd_process_v_(engine_);
                }

                ASSERT(p < STANDING + ACCELERATION + UNIFORM + DECELERATION);
                const T t = p - (STANDING + ACCELERATION + UNIFORM);
                return process_acceleration_ * ACCELERATION - (ACCELERATION / DECELERATION) * process_acceleration_ * t
                       + nd_process_v_(engine_);
        }

public:
        AccelerationSimulator(
                const T init_x,
                const T dt,
                const T process_acceleration,
                const T process_velocity_variance,
                const T measurement_variance_x,
                const T measurement_variance_v)
                : dt_(dt),
                  process_acceleration_(process_acceleration),
                  measurement_variance_x_(measurement_variance_x),
                  measurement_variance_v_(measurement_variance_v),
                  nd_process_v_(0, std::sqrt(process_velocity_variance)),
                  nd_measurement_x_(0, std::sqrt(measurement_variance_x)),
                  nd_measurement_v_(0, std::sqrt(measurement_variance_v)),
                  x_(init_x)
        {
        }

        void move()
        {
                ++index_;
                time_ = index_ * dt_;
                const T v_next = speed();
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
                const T v = v_ + (v_ > 0 ? nd_measurement_v_(engine_) : 0);
                return {.value = v, .variance = measurement_variance_v_};
        }
};

template <typename T>
std::vector<Measurements<T>> simulate(const T length, AccelerationSimulator<T>* const simulator)
{
        std::vector<Measurements<T>> res;

        while (simulator->time() <= length)
        {
                res.push_back(
                        {.time = simulator->time(),
                         .true_position = simulator->x(),
                         .true_speed = simulator->v(),
                         .position = simulator->measurement_x(),
                         .speed = simulator->measurement_v()});

                simulator->move();
        }

        return res;
}
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
        AccelerationSimulator simulator{
                init_x,
                dt,
                process_acceleration,
                process_velocity_variance,
                measurement_variance_x,
                measurement_variance_v};

        return simulate(length, &simulator);
}

#define INSTANTIATION(T) template std::vector<Measurements<T>> simulate_acceleration(T, T, T, T, T, T, T);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
