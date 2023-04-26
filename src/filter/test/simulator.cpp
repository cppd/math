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

#include <src/com/conversion.h>
#include <src/com/random/pcg.h>

#include <cmath>
#include <random>

namespace ns::filter::test
{
namespace
{
template <typename T, typename Angle>
Vector<2, T> rotate(const Vector<2, T>& v, const Angle angle)
{
        static_assert(std::is_floating_point_v<Angle>);
        return {std::cos(angle) * v[0] - std::sin(angle) * v[1], std::sin(angle) * v[0] + std::cos(angle) * v[1]};
}

template <std::size_t N, typename T>
class Simulator final
{
        const T dt_;
        const T track_velocity_mean_;

        PCG engine_;
        std::normal_distribution<T> track_velocity_nd_;
        std::normal_distribution<T> measurements_velocity_amount_nd_;
        std::normal_distribution<T> measurements_velocity_direction_nd_;
        std::normal_distribution<T> measurements_position_nd_;

        std::size_t index_{0};
        Vector<N, T> position_{0};
        Vector<N, T> velocity_{0};

public:
        Simulator(
                const std::type_identity_t<T> dt,
                const std::type_identity_t<T> track_velocity_mean,
                const std::type_identity_t<T> track_velocity_variance,
                const std::type_identity_t<T> measurements_velocity_amount_variance,
                const std::type_identity_t<T> measurements_velocity_direction_variance,
                const std::type_identity_t<T> measurements_position_variance)
                : dt_(dt),
                  track_velocity_mean_(track_velocity_mean),
                  track_velocity_nd_(0, std::sqrt(track_velocity_variance)),
                  measurements_velocity_amount_nd_(0, std::sqrt(measurements_velocity_amount_variance)),
                  measurements_velocity_direction_nd_(0, std::sqrt(measurements_velocity_direction_variance)),
                  measurements_position_nd_(0, std::sqrt(measurements_position_variance))
        {
        }

        void move()
        {
                Vector<N, T> v(0);
                v[0] = track_velocity_mean_;
                velocity_ = rotate(v, std::cos(index_ / T{100}));
                for (std::size_t i = 0; i < N; ++i)
                {
                        velocity_[i] += track_velocity_nd_(engine_);
                }
                position_ += dt_ * velocity_;
                ++index_;
        }

        [[nodiscard]] const Vector<N, T>& position() const
        {
                return position_;
        }

        [[nodiscard]] VelocityMeasurement<N, T> velocity_measurement()
        {
                const T amount = measurements_velocity_amount_nd_(engine_);
                const T direction = measurements_velocity_direction_nd_(engine_);
                return {.direction = rotate(velocity_, -T{0.5} - index_ / T{600} * degrees_to_radians(T{5}) + direction)
                                             .normalized(),
                        .amount = velocity_.norm() + amount};
        }

        [[nodiscard]] PositionMeasurement<N, T> position_measurement()
        {
                Vector<N, T> position{position_};
                for (std::size_t i = 0; i < N; ++i)
                {
                        position[i] += measurements_position_nd_(engine_);
                }
                return {.position = position};
        }
};
}

template <std::size_t N, typename T>
Track<N, T> generate_track(
        const std::size_t count,
        const T dt,
        const T track_velocity_mean,
        const T track_velocity_variance,
        const T measurements_velocity_amount_variance,
        const T measurements_velocity_direction_variance,
        const T measurements_position_variance,
        const std::size_t position_interval)
{
        Simulator<N, T> simulator(
                dt, track_velocity_mean, track_velocity_variance, measurements_velocity_amount_variance,
                measurements_velocity_direction_variance, measurements_position_variance);

        Track<N, T> res;
        res.positions.reserve(count);
        res.position_measurements.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
        {
                simulator.move();

                res.positions.push_back(simulator.position());

                res.velocity_measurements.push_back(simulator.velocity_measurement());

                if ((i % position_interval) == 0)
                {
                        res.position_measurements[i] = simulator.position_measurement();
                }
        }

        return res;
}

#define TEMPLATE(T) template Track<2, T> generate_track(std::size_t, T, T, T, T, T, T, std::size_t);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
