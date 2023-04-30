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

#include <src/com/conversion.h>
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
        const T track_velocity_mean_;

        PCG engine_;

        std::normal_distribution<T> track_velocity_nd_;

        std::normal_distribution<T> measurements_velocity_amount_nd_;
        std::normal_distribution<T> measurements_velocity_direction_nd_;
        std::normal_distribution<T> measurements_acceleration_nd_;
        std::normal_distribution<T> measurements_position_nd_;
        std::normal_distribution<T> measurements_position_speed_nd_;

        std::size_t index_{0};
        Vector<N, T> position_{0};
        Vector<N, T> velocity_;
        Vector<N, T> next_velocity_;
        Vector<N, T> acceleration_;

        [[nodiscard]] Vector<N, T> velocity(const T index) const
        {
                Vector<N, T> v(0);
                v[0] = track_velocity_mean_;
                return rotate(v, std::cos(index / 100));
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
                const std::type_identity_t<T> track_velocity_mean,
                const std::type_identity_t<T> track_velocity_variance,
                const TrackMeasurementVariance<T>& track_measurement_variance)
                : dt_(dt),
                  track_velocity_mean_(track_velocity_mean),
                  track_velocity_nd_(0, std::sqrt(track_velocity_variance)),
                  measurements_velocity_amount_nd_(0, std::sqrt(track_measurement_variance.velocity_amount)),
                  measurements_velocity_direction_nd_(0, std::sqrt(track_measurement_variance.velocity_direction)),
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
                acceleration_ = next_velocity_ - velocity_;
        }

        [[nodiscard]] const Vector<N, T>& position() const
        {
                return position_;
        }

        [[nodiscard]] ProcessMeasurement<N, T> process_measurement()
        {
                const T angle = -T{0.5} - index_ / T{600} * degrees_to_radians(T{5});
                const Vector<N, T> direction = rotate(velocity_, angle + measurements_velocity_direction_nd_(engine_));
                return ProcessMeasurement<N, T>{
                        .direction = direction.normalized(),
                        .amount = velocity_.norm() + measurements_velocity_amount_nd_(engine_),
                        .acceleration = rotate(acceleration_ + vector(measurements_acceleration_nd_), angle)};
        }

        [[nodiscard]] PositionMeasurement<N, T> position_measurement()
        {
                return {.position = position_ + vector(measurements_position_nd_),
                        .speed = velocity_.norm() + measurements_position_speed_nd_(engine_)};
        }
};
}

template <std::size_t N, typename T>
Track<N, T> generate_track(
        const std::size_t count,
        const T dt,
        const T track_velocity_mean,
        const T track_velocity_variance,
        const TrackMeasurementVariance<T>& track_measurement_variance,
        const std::size_t position_interval)
{
        Simulator<N, T> simulator(dt, track_velocity_mean, track_velocity_variance, track_measurement_variance);

        Track<N, T> res;
        res.positions.reserve(count);
        res.position_measurements.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
        {
                simulator.move();

                res.positions.push_back(simulator.position());

                res.process_measurements.push_back(simulator.process_measurement());

                if ((i % position_interval) == 0)
                {
                        res.position_measurements[i] = simulator.position_measurement();
                }
        }

        return res;
}

#define TEMPLATE(T) \
        template Track<2, T> generate_track(std::size_t, T, T, T, const TrackMeasurementVariance<T>&, std::size_t);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
