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

#include "utility.h"

#include <src/com/constant.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/random/pcg.h>

#include <cmath>
#include <random>

namespace ns::filter::test
{
namespace
{
template <std::size_t N, typename T>
class Simulator final
{
        const T dt_;
        const T track_velocity_m_;
        const T track_velocity_a_;
        const T direction_bias_drift_;
        const T direction_angle_;

        PCG engine_;

        std::normal_distribution<T> track_velocity_nd_;

        std::normal_distribution<T> measurements_velocity_direction_nd_;
        std::normal_distribution<T> measurements_acceleration_nd_;
        std::normal_distribution<T> measurements_position_nd_;
        std::normal_distribution<T> measurements_position_speed_nd_;

        std::size_t index_{0};
        Vector<N, T> position_{0};
        Vector<N, T> velocity_;
        Vector<N, T> next_velocity_;
        Vector<N, T> acceleration_;
        T angle_;

        [[nodiscard]] Vector<N, T> velocity(const T index) const
        {
                constexpr T S = 1000;
                Vector<N, T> v(0);
                v[0] = track_velocity_m_ + track_velocity_a_ * std::sin(index * (PI<T> / 300));
                return rotate(v, std::cos((std::max<T>(0, index - S)) * (PI<T> / 450)));
        }

        [[nodiscard]] Vector<N, T> vector(std::normal_distribution<T>& distribution)
        {
                Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = distribution(engine_);
                }
                return res;
        }

public:
        Simulator(
                const std::type_identity_t<T> dt,
                const std::type_identity_t<T> track_velocity_min,
                const std::type_identity_t<T> track_velocity_max,
                const std::type_identity_t<T> track_velocity_variance,
                const T direction_bias_drift,
                const T direction_angle,
                const TrackMeasurementVariance<T>& track_measurement_variance)
                : dt_(dt),
                  track_velocity_m_((track_velocity_min + track_velocity_max) / 2),
                  track_velocity_a_((track_velocity_max - track_velocity_min) / 2),
                  direction_bias_drift_(direction_bias_drift / (T{60} * T{60}) * dt_),
                  direction_angle_(direction_angle),
                  track_velocity_nd_(0, std::sqrt(track_velocity_variance)),
                  measurements_velocity_direction_nd_(0, std::sqrt(track_measurement_variance.direction)),
                  measurements_acceleration_nd_(0, std::sqrt(track_measurement_variance.acceleration)),
                  measurements_position_nd_(0, std::sqrt(track_measurement_variance.position)),
                  measurements_position_speed_nd_(0, std::sqrt(track_measurement_variance.position_speed)),
                  velocity_(velocity(index_) + vector(track_velocity_nd_)),
                  next_velocity_(velocity(index_ + 1) + vector(track_velocity_nd_)),
                  acceleration_(next_velocity_ - velocity_)
        {
        }

        void move()
        {
                ++index_;

                position_ = position_ + dt_ * velocity_ + (square(dt_) / 2) * acceleration_;

                velocity_ = next_velocity_;
                next_velocity_ = velocity(index_ + 1) + vector(track_velocity_nd_);
                acceleration_ = (next_velocity_ - velocity_) / dt_;

                angle_ = -T{3} - index_ * direction_bias_drift_;
        }

        [[nodiscard]] const Vector<N, T>& position() const
        {
                return position_;
        }

        [[nodiscard]] T speed() const
        {
                return velocity_.norm();
        }

        [[nodiscard]] T angle() const
        {
                return angle_;
        }

        [[nodiscard]] T angle_r() const
        {
                return direction_angle_;
        }

        [[nodiscard]] ProcessMeasurement<N, T> process_measurement()
        {
                const Vector<N, T> direction =
                        rotate(velocity_, direction_angle_ + angle_ + measurements_velocity_direction_nd_(engine_));
                return ProcessMeasurement<N, T>{
                        .direction = std::atan2(direction[1], direction[0]),
                        .acceleration = rotate(acceleration_ + vector(measurements_acceleration_nd_), angle_)};
        }

        [[nodiscard]] Vector<N, T> position_measurement()
        {
                return position_ + vector(measurements_position_nd_);
        }

        [[nodiscard]] T speed_measurement()
        {
                return velocity_.norm() + measurements_position_speed_nd_(engine_);
        }
};
}

template <std::size_t N, typename T>
Track<N, T> generate_track(
        const std::size_t count,
        const T dt,
        const T track_velocity_min,
        const T track_velocity_max,
        const T track_velocity_variance,
        const T direction_bias_drift,
        const T direction_angle,
        const TrackMeasurementVariance<T>& track_measurement_variance,
        const std::size_t position_interval)
{
        ASSERT(track_velocity_min >= 0);
        ASSERT(track_velocity_max >= track_velocity_min);

        Simulator<N, T> simulator(
                dt, track_velocity_min, track_velocity_max, track_velocity_variance, direction_bias_drift,
                direction_angle, track_measurement_variance);

        Track<N, T> res;
        res.points.reserve(count);
        res.process_measurements.reserve(count);
        res.position_measurements.reserve(count / position_interval);

        for (std::size_t i = 0; i < count; ++i)
        {
                simulator.move();

                res.points.push_back(
                        {.position = simulator.position(),
                         .speed = simulator.speed(),
                         .angle = simulator.angle(),
                         .angle_r = simulator.angle_r()});

                res.process_measurements.push_back(simulator.process_measurement());

                if ((i % position_interval) == 0)
                {
                        res.position_measurements.push_back(
                                {.index = i,
                                 .position = simulator.position_measurement(),
                                 .speed = simulator.speed_measurement()});
                }
        }

        return res;
}

#define TEMPLATE(T)                          \
        template Track<2, T> generate_track( \
                std::size_t, T, T, T, T, T, T, const TrackMeasurementVariance<T>&, std::size_t);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
