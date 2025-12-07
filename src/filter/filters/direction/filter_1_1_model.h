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

#include "init.h"

#include <src/com/angle.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/variant.h>
#include <src/filter/core/kinematic_models.h>
#include <src/filter/filters/noise_model.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::filter::filters::direction::filter_1_1_model
{
template <typename T>
numerical::Vector<6, T> x(const numerical::Vector<4, T>& position_velocity, const Init<T>& init)
{
        ASSERT(is_finite(position_velocity));

        numerical::Vector<6, T> res;

        res[0] = position_velocity[0];
        res[1] = position_velocity[1];
        res[2] = position_velocity[2];
        res[3] = position_velocity[3];
        res[4] = init.angle;
        res[5] = init.angle_speed;

        return res;
}

template <typename T>
numerical::Matrix<6, 6, T> p(const numerical::Matrix<4, 4, T>& position_velocity_p, const Init<T>& init)
{
        ASSERT(is_finite(position_velocity_p));

        numerical::Matrix<6, 6, T> res(numerical::ZERO_MATRIX);

        for (std::size_t r = 0; r < 4; ++r)
        {
                for (std::size_t c = 0; c < 4; ++c)
                {
                        res[r, c] = position_velocity_p[r, c];
                }
        }

        res[4, 4] = init.angle_variance;
        res[5, 5] = init.angle_speed_variance;

        return res;
}

template <typename T>
numerical::Vector<6, T> add_x(const numerical::Vector<6, T>& a, const numerical::Vector<6, T>& b)
{
        numerical::Vector<6, T> res = a + b;
        res[4] = wrap_angle(res[4]);
        return res;
}

template <typename T>
numerical::Vector<6, T> f(const T dt, const numerical::Vector<6, T>& x)
{
        const T px = x[0];
        const T vx = x[1];
        const T py = x[2];
        const T vy = x[3];
        const T angle = x[4];
        const T angle_v = x[5];

        return {
                px + dt * vx, // px
                vx, // vx
                py + dt * vy, // py
                vy, // vy
                angle + dt * angle_v, // angle
                angle_v // angle_v
        };
}

template <typename T>
numerical::Matrix<6, 6, T> q(
        const T dt,
        const NoiseModel<T>& position_noise_model,
        const NoiseModel<T>& angle_noise_model)
{
        const auto position = std::visit(
                Visitors{
                        [&](const ContinuousNoiseModel<T>& model)
                        {
                                return core::continuous_white_noise<2, T>(dt, model.spectral_density);
                        },
                        [&](const DiscreteNoiseModel<T>& model)
                        {
                                const T dt_2 = power<2>(dt) / 2;
                                const numerical::Matrix<2, 1, T> noise_transition{{dt_2}, {dt}};
                                const numerical::Matrix<1, 1, T> process_covariance = {{model.variance}};
                                return noise_transition * process_covariance * noise_transition.transposed();
                        }},
                position_noise_model);

        const auto angle = std::visit(
                Visitors{
                        [&](const ContinuousNoiseModel<T>& model)
                        {
                                return core::continuous_white_noise<2, T>(dt, model.spectral_density);
                        },
                        [&](const DiscreteNoiseModel<T>& model)
                        {
                                const T dt_2 = power<2>(dt) / 2;
                                const numerical::Matrix<2, 1, T> noise_transition{{dt_2}, {dt}};
                                const numerical::Matrix<1, 1, T> process_covariance = {{model.variance}};
                                return noise_transition * process_covariance * noise_transition.transposed();
                        }},
                angle_noise_model);

        return numerical::block_diagonal(position, position, angle);
}
}
