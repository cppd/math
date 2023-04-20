/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/com/random/pcg.h>

#include <cmath>
#include <random>

namespace ns::filter::test
{
namespace
{
template <typename T>
Vector<2, T> rotate(const Vector<2, T>& v, const T angle)
{
        return {std::cos(angle) * v[0] - std::sin(angle) * v[1], std::sin(angle) * v[0] + std::cos(angle) * v[1]};
}

template <std::size_t N, typename T>
class Simulator final
{
        const T dt_;
        const T velocity_mean_;

        PCG engine_;
        std::normal_distribution<T> nd_velocity_;
        std::normal_distribution<T> nd_relative_direction_measurements_;
        std::normal_distribution<T> nd_relative_amount_measurements_;
        std::normal_distribution<T> nd_velocity_measurements_;
        std::normal_distribution<T> nd_position_measurements_;

        Vector<N, T> position_{0};
        Vector<N, T> velocity_{0};

public:
        Simulator(
                const std::type_identity_t<T> dt,
                const std::type_identity_t<T> velocity_mean,
                const std::type_identity_t<T> velocity_variance,
                const std::type_identity_t<T> relative_direction_measurements_variance,
                const std::type_identity_t<T> relative_amount_measurements_variance,
                const std::type_identity_t<T> velocity_measurements_variance,
                const std::type_identity_t<T> position_measurements_variance)
                : dt_(dt),
                  velocity_mean_(velocity_mean),
                  nd_velocity_(0, std::sqrt(velocity_variance)),
                  nd_relative_direction_measurements_(0, std::sqrt(relative_direction_measurements_variance)),
                  nd_relative_amount_measurements_(0, std::sqrt(relative_amount_measurements_variance)),
                  nd_velocity_measurements_(0, std::sqrt(velocity_measurements_variance)),
                  nd_position_measurements_(0, std::sqrt(position_measurements_variance))
        {
        }

        void move()
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        velocity_[i] = velocity_mean_ + nd_velocity_(engine_);
                }
                position_ += dt_ * velocity_;
        }

        [[nodiscard]] const Vector<N, T>& position() const
        {
                return position_;
        }

        [[nodiscard]] RelativeMeasurement<N, T> relative_measurement()
        {
                const T direction = nd_relative_direction_measurements_(engine_);
                const T amount = nd_relative_amount_measurements_(engine_);
                return {.direction = rotate(velocity_, direction).normalized(), .amount = velocity_.norm() + amount};
        }

        [[nodiscard]] Vector<N, T> velocity_measurement()
        {
                Vector<N, T> res{velocity_};
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] += nd_velocity_measurements_(engine_);
                }
                return res;
        }

        [[nodiscard]] Vector<N, T> position_measurement()
        {
                Vector<N, T> res{position_};
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] += nd_position_measurements_(engine_);
                }
                return res;
        }
};
}

template <std::size_t N, typename T>
Track<N, T> generate_track(
        const std::size_t count,
        const T dt,
        const T track_velocity_mean,
        const T track_velocity_variance,
        const T relative_direction_variance,
        const T relative_amount_variance,
        const T velocity_variance,
        const T position_variance,
        const std::size_t position_interval)
{
        Simulator<N, T> simulator(
                dt, track_velocity_mean, track_velocity_variance, relative_direction_variance, relative_amount_variance,
                velocity_variance, position_variance);

        Track<N, T> res;
        res.positions.reserve(count);
        res.velocity_measurements.reserve(count);
        res.position_measurements.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
        {
                simulator.move();

                res.positions.push_back(simulator.position());

                res.relative_measurements.push_back(simulator.relative_measurement());
                res.velocity_measurements.push_back(simulator.velocity_measurement());

                if ((i % position_interval) == 0)
                {
                        res.position_measurements[i] = simulator.position_measurement();
                }
        }

        return res;
}

#define TEMPLATE(T) template Track<2, T> generate_track(std::size_t, T, T, T, T, T, T, T, std::size_t);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
