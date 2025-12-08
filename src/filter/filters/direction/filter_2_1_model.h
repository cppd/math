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

#pragma once

#include "init.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/variant.h>
#include <src/filter/core/angle.h>
#include <src/filter/core/kinematic_models.h>
#include <src/filter/filters/noise_model.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::filter::filters::direction::filter_2_1_model
{
template <typename T>
numerical::Vector<8, T> x(const numerical::Vector<4, T>& position_velocity, const Init<T>& init)
{
        ASSERT(is_finite(position_velocity));

        numerical::Vector<8, T> res;

        res[0] = position_velocity[0];
        res[1] = position_velocity[1];
        res[2] = init.acceleration;
        res[3] = position_velocity[2];
        res[4] = position_velocity[3];
        res[5] = init.acceleration;
        res[6] = init.angle;
        res[7] = init.angle_speed;

        return res;
}

template <typename T>
numerical::Matrix<8, 8, T> p(const numerical::Matrix<4, 4, T>& position_velocity_p, const Init<T>& init)
{
        ASSERT(is_finite(position_velocity_p));

        const numerical::Matrix<4, 4, T>& p = position_velocity_p;
        static constexpr std::size_t N = 2;

        numerical::Matrix<8, 8, T> res(numerical::ZERO_MATRIX);

        for (std::size_t r = 0; r < N; ++r)
        {
                for (std::size_t i = 0; i < 2; ++i)
                {
                        for (std::size_t c = 0; c < N; ++c)
                        {
                                for (std::size_t j = 0; j < 2; ++j)
                                {
                                        res[3 * r + i, 3 * c + j] = p[2 * r + i, 2 * c + j];
                                }
                        }
                }
        }

        res[2, 2] = init.acceleration_variance;
        res[5, 5] = init.acceleration_variance;
        res[6, 6] = init.angle_variance;
        res[7, 7] = init.angle_speed_variance;

        return res;
}

template <typename T>
numerical::Vector<8, T> add_x(const numerical::Vector<8, T>& a, const numerical::Vector<8, T>& b)
{
        numerical::Vector<8, T> res = a + b;
        res[6] = core::wrap_angle(res[6]);
        return res;
}

template <typename T>
numerical::Vector<8, T> f(const T dt, const numerical::Vector<8, T>& x)
{
        const T dt_2 = square(dt) / 2;

        const T px = x[0];
        const T vx = x[1];
        const T ax = x[2];
        const T py = x[3];
        const T vy = x[4];
        const T ay = x[5];
        const T angle = x[6];
        const T angle_v = x[7];

        return {
                px + dt * vx + dt_2 * ax, // px
                vx + dt * ax, // vx
                ax, // ax
                py + dt * vy + dt_2 * ay, // py
                vy + dt * ay, // vy
                ay, // ay
                angle + dt * angle_v, // angle
                angle_v // angle_v
        };
}

template <typename T>
numerical::Matrix<8, 8, T> q(
        const T dt,
        const NoiseModel<T>& position_noise_model,
        const NoiseModel<T>& angle_noise_model)
{
        const auto position = std::visit(
                Visitors{
                        [&](const ContinuousNoiseModel<T>& model)
                        {
                                return core::continuous_white_noise<3, T>(dt, model.spectral_density);
                        },
                        [&](const DiscreteNoiseModel<T>& model)
                        {
                                const T dt_2 = power<2>(dt) / 2;
                                const T dt_3 = power<3>(dt) / 6;
                                const numerical::Matrix<3, 1, T> noise_transition{{dt_3}, {dt_2}, {dt}};
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
