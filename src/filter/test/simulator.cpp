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

#include <random>

namespace ns::filter::test
{
namespace
{
template <std::size_t N, typename T>
class Simulator final
{
        const T dt_;
        const T velocity_mean_;

        PCG engine_;
        std::normal_distribution<T> nd_velocity_;
        std::normal_distribution<T> nd_velocity_measurements_;
        std::normal_distribution<T> nd_position_measurements_;

        Vector<N, T> position_{0};
        Vector<N, T> velocity_{0};

public:
        Simulator(
                const std::type_identity_t<T> dt,
                const std::type_identity_t<T> velocity_mean,
                const std::type_identity_t<T> velocity_variance,
                const std::type_identity_t<T> velocity_measurements_variance,
                const std::type_identity_t<T> position_measurements_variance)
                : dt_(dt),
                  velocity_mean_(velocity_mean),
                  nd_velocity_(0, std::sqrt(velocity_variance)),
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
        const T velocity_variance,
        const T position_variance,
        const std::size_t position_interval)
{
        Simulator<N, T> simulator(
                dt, track_velocity_mean, track_velocity_variance, velocity_variance, position_variance);

        Track<N, T> res;
        res.positions.reserve(count);
        res.velocity_measurements.reserve(count);
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

#define TEMPLATE(T) template Track<2, T> generate_track(std::size_t, T, T, T, T, T, std::size_t);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
