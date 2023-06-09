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
        struct Velocity final
        {
                T magnitude;
                T angle;
        };

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

        T time_{0};
        Vector<N, T> position_{0};
        Velocity velocity_;
        Velocity next_velocity_;
        Vector<N, T> acceleration_;
        T angle_;

        [[nodiscard]] Velocity velocity_with_noise(const T time)
        {
                static constexpr T S = 100;
                const T magnitude = std::max<T>(0, speed_m_ + speed_a_ * std::sin(time * (PI<T> / 30)));
                const T angle = std::cos((std::max<T>(0, time - S)) * (PI<T> / 45));
                return {.magnitude = magnitude + speed_nd_(engine_), .angle = angle};
        }

        [[nodiscard]] Vector<2, T> to_vector(const Velocity& v) const
        {
                return {v.magnitude * std::cos(v.angle), v.magnitude * std::sin(v.angle)};
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
        explicit Simulator(const TrackInfo<T>& info)
                : dt_(info.measurement_dt),
                  speed_m_((info.track_speed_min + info.track_speed_max) / 2),
                  speed_a_((info.track_speed_max - info.track_speed_min) / 2),
                  direction_bias_drift_(info.track_direction_bias_drift / (T{60} * T{60})),
                  direction_angle_(info.track_direction_angle),
                  speed_nd_(0, std::sqrt(info.track_speed_variance)),
                  measurements_direction_nd_(0, std::sqrt(info.measurement_variance_direction)),
                  measurements_acceleration_nd_(0, std::sqrt(info.measurement_variance_acceleration)),
                  measurements_position_nd_(0, std::sqrt(info.measurement_variance_position)),
                  measurements_speed_nd_(0, std::sqrt(info.measurement_variance_speed)),
                  velocity_(velocity_with_noise(time_)),
                  next_velocity_(velocity_with_noise(time_ + dt_)),
                  acceleration_((to_vector(next_velocity_) - to_vector(velocity_)) / dt_)
        {
        }

        void move()
        {
                time_ += dt_;

                position_ = position_ + dt_ * to_vector(velocity_) + (square(dt_) / 2) * acceleration_;

                velocity_ = next_velocity_;
                next_velocity_ = velocity_with_noise(time_ + dt_);
                acceleration_ = (to_vector(next_velocity_) - to_vector(velocity_)) / dt_;

                angle_ = -T{3} - time_ * direction_bias_drift_;
        }

        [[nodiscard]] const Vector<N, T>& position() const
        {
                return position_;
        }

        [[nodiscard]] T speed() const
        {
                return velocity_.magnitude;
        }

        [[nodiscard]] T angle() const
        {
                return angle_;
        }

        [[nodiscard]] T angle_r() const
        {
                return direction_angle_;
        }

        [[nodiscard]] T measurement_direction()
        {
                return velocity_.angle + direction_angle_ + angle_ + measurements_direction_nd_(engine_);
        }

        [[nodiscard]] Vector<N, T> measurement_acceleration()
        {
                return rotate(acceleration_, angle_) + vector(measurements_acceleration_nd_);
        }

        [[nodiscard]] Vector<N, T> measurement_position()
        {
                return position_ + vector(measurements_position_nd_);
        }

        [[nodiscard]] T measurement_speed()
        {
                return velocity_.magnitude + measurements_speed_nd_(engine_);
        }
};
}

template <std::size_t N, typename T>
Track<N, T> generate_track(const std::size_t count, const TrackInfo<T>& info)
{
        ASSERT(info.track_speed_max >= info.track_speed_min);
        ASSERT(info.measurement_dt_count_acceleration > 0 && info.measurement_dt_count_direction > 0
               && info.measurement_dt_count_position > 0 && info.measurement_dt_count_speed > 0);

        Simulator<N, T> simulator(info);

        Track<N, T> res;
        res.points.reserve(count);
        res.measurements.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
        {
                simulator.move();

                res.points.push_back(
                        {.time = i * info.measurement_dt,
                         .position = simulator.position(),
                         .speed = simulator.speed(),
                         .angle = simulator.angle(),
                         .angle_r = simulator.angle_r()});

                ProcessMeasurement<N, T>& m = res.measurements.emplace_back();

                m.simulator_point_index = i;
                m.time = i * info.measurement_dt;

                if (i % info.measurement_dt_count_acceleration == 0)
                {
                        m.acceleration = simulator.measurement_acceleration();
                }

                if (i % info.measurement_dt_count_direction == 0)
                {
                        m.direction = simulator.measurement_direction();
                }

                if (i % info.measurement_dt_count_position == 0)
                {
                        m.position = simulator.measurement_position();
                }

                if (i % info.measurement_dt_count_speed == 0)
                {
                        m.speed = simulator.measurement_speed();
                }
        }

        return res;
}

#define TEMPLATE(T) template Track<2, T> generate_track(std::size_t, const TrackInfo<T>&);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
