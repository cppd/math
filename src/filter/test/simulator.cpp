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
        const T speed_m_;
        const T speed_a_;
        const T direction_bias_drift_;
        const T direction_angle_;

        PCG engine_;

        std::normal_distribution<T> speed_nd_;

        std::normal_distribution<T> measurements_direction_nd_;
        std::normal_distribution<T> measurements_acceleration_nd_;
        std::normal_distribution<T> measurements_position_nd_;
        std::normal_distribution<T> measurements_speed_nd_;

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
                v[0] = speed_m_ + speed_a_ * std::sin(index * (PI<T> / 300));
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
        Simulator(const TrackInfo<T>& info, const TrackMeasurementVariance<T>& track_measurement_variance)
                : dt_(info.dt),
                  speed_m_((info.speed_min + info.speed_max) / 2),
                  speed_a_((info.speed_max - info.speed_min) / 2),
                  direction_bias_drift_(info.direction_bias_drift / (T{60} * T{60}) * dt_),
                  direction_angle_(info.direction_angle),
                  speed_nd_(0, std::sqrt(info.speed_variance)),
                  measurements_direction_nd_(0, std::sqrt(track_measurement_variance.direction)),
                  measurements_acceleration_nd_(0, std::sqrt(track_measurement_variance.acceleration)),
                  measurements_position_nd_(0, std::sqrt(track_measurement_variance.position)),
                  measurements_speed_nd_(0, std::sqrt(track_measurement_variance.speed)),
                  velocity_(velocity(index_) + vector(speed_nd_)),
                  next_velocity_(velocity(index_ + 1) + vector(speed_nd_)),
                  acceleration_(next_velocity_ - velocity_)
        {
        }

        void move()
        {
                ++index_;

                position_ = position_ + dt_ * velocity_ + (square(dt_) / 2) * acceleration_;

                velocity_ = next_velocity_;
                next_velocity_ = velocity(index_ + 1) + vector(speed_nd_);
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

        [[nodiscard]] T process_direction()
        {
                const Vector<N, T> direction =
                        rotate(velocity_, direction_angle_ + angle_ + measurements_direction_nd_(engine_));
                return std::atan2(direction[1], direction[0]);
        }

        [[nodiscard]] Vector<N, T> process_acceleration()
        {
                return rotate(acceleration_ + vector(measurements_acceleration_nd_), angle_);
        }

        [[nodiscard]] Vector<N, T> position_measurement()
        {
                return position_ + vector(measurements_position_nd_);
        }

        [[nodiscard]] T speed_measurement()
        {
                return velocity_.norm() + measurements_speed_nd_(engine_);
        }
};
}

template <std::size_t N, typename T>
Track<N, T> generate_track(
        const std::size_t count,
        const TrackInfo<T>& info,
        const TrackMeasurementVariance<T>& measurement_variance)
{
        ASSERT(info.speed_min >= 0);
        ASSERT(info.speed_max >= info.speed_min);

        Simulator<N, T> simulator(info, measurement_variance);

        Track<N, T> res;
        res.points.reserve(count);
        res.process_measurements.reserve(count);
        res.position_measurements.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
        {
                simulator.move();

                res.points.push_back(
                        {.time = i * info.dt,
                         .position = simulator.position(),
                         .speed = simulator.speed(),
                         .angle = simulator.angle(),
                         .angle_r = simulator.angle_r()});

                res.process_measurements.push_back(
                        {.simulator_point_index = i,
                         .time = i * info.dt,
                         .direction = simulator.process_direction(),
                         .acceleration = simulator.process_acceleration()});

                res.position_measurements.push_back(
                        {.simulator_point_index = i,
                         .time = i * info.dt,
                         .position = simulator.position_measurement(),
                         .speed = simulator.speed_measurement()});
        }

        return res;
}

#define TEMPLATE(T) \
        template Track<2, T> generate_track(std::size_t, const TrackInfo<T>&, const TrackMeasurementVariance<T>&);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
