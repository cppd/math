/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <src/filter/core/kinematic_models.h>
#include <src/filter/filters/noise_model.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::filter::filters::position::filter_2_model
{
template <std::size_t N, typename T>
numerical::Vector<3 * N, T> x(const numerical::Vector<N, T>& position, const Init<T>& init)
{
        ASSERT(is_finite(position));

        numerical::Vector<3 * N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 3 * i;
                res[b + 0] = position[i];
                res[b + 1] = init.speed;
                res[b + 2] = init.acceleration;
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Matrix<3 * N, 3 * N, T> p(const numerical::Vector<N, T>& position_variance, const Init<T>& init)
{
        ASSERT(is_finite(position_variance));

        numerical::Matrix<3 * N, 3 * N, T> res(0);
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 3 * i;
                res[b + 0, b + 0] = position_variance[i];
                res[b + 1, b + 1] = init.speed_variance;
                res[b + 2, b + 2] = init.acceleration_variance;
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Vector<N, T> add_x(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b)
{
        return a + b;
}

template <std::size_t N, typename T>
numerical::Matrix<3 * N, 3 * N, T> f(const T dt)
{
        const T dt_2 = power<2>(dt) / 2;

        return block_diagonal<N>(numerical::Matrix<3, 3, T>{
                {1, dt, dt_2},
                {0,  1,   dt},
                {0,  0,    1}
        });
}

template <std::size_t N, typename T>
numerical::Matrix<3 * N, 3 * N, T> q(const T dt, const NoiseModel<T>& noise_model)
{
        const auto visitors = Visitors{
                [&](const ContinuousNoiseModel<T>& model)
                {
                        return block_diagonal<N>(core::continuous_white_noise<3, T>(dt, model.spectral_density));
                },
                [&](const DiscreteNoiseModel<T>& model)
                {
                        const T dt_2 = power<2>(dt) / 2;
                        const T dt_3 = power<3>(dt) / 6;

                        const numerical::Matrix<3 * N, N, T> noise_transition =
                                block_diagonal<N>(numerical::Matrix<3, 1, T>{{dt_3}, {dt_2}, {dt}});
                        const numerical::Matrix<N, N, T> process_covariance =
                                numerical::make_diagonal_matrix(numerical::Vector<N, T>(model.variance));

                        return noise_transition * process_covariance * noise_transition.transposed();
                }};
        return std::visit(visitors, noise_model);
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> position_r(const numerical::Vector<N, T>& measurement_variance)
{
        return numerical::make_diagonal_matrix(measurement_variance);
}

template <std::size_t N, typename T>
numerical::Vector<N / 3, T> position_h(const numerical::Vector<N, T>& x)
{
        static_assert(N % 3 == 0);
        // px = px
        // py = py
        numerical::Vector<N / 3, T> res;
        for (std::size_t i = 0; i < N / 3; ++i)
        {
                res[i] = x[3 * i];
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Matrix<N / 3, N, T> position_hj(const numerical::Vector<N, T>& /*x*/)
{
        static_assert(N % 3 == 0);
        // px = px
        // py = py
        // Jacobian
        numerical::Matrix<N / 3, N, T> res(0);
        for (std::size_t i = 0; i < N / 3; ++i)
        {
                res[i, 3 * i] = 1;
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Vector<N, T> position_residual(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b)
{
        return a - b;
}
}
