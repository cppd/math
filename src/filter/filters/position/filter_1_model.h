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

namespace ns::filter::filters::position::filter_1_model
{
template <std::size_t N, typename T>
numerical::Vector<2 * N, T> x(const numerical::Vector<N, T>& position, const Init<T>& init)
{
        ASSERT(is_finite(position));

        numerical::Vector<2 * N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 2 * i;
                res[b + 0] = position[i];
                res[b + 1] = init.speed;
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Matrix<2 * N, 2 * N, T> p(const numerical::Vector<N, T>& position_variance, const Init<T>& init)
{
        ASSERT(is_finite(position_variance));

        numerical::Matrix<2 * N, 2 * N, T> res(0);
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 2 * i;
                res[b + 0, b + 0] = position_variance[i];
                res[b + 1, b + 1] = init.speed_variance;
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Vector<N, T> add_x(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b)
{
        return a + b;
}

template <std::size_t N, typename T>
numerical::Matrix<2 * N, 2 * N, T> f(const T dt)
{
        return block_diagonal<N>(numerical::Matrix<2, 2, T>{
                {1, dt},
                {0,  1}
        });
}

template <std::size_t N, typename T>
numerical::Matrix<2 * N, 2 * N, T> q(const T dt, const NoiseModel<T>& noise_model)
{
        const auto visitors = Visitors{
                [&](const ContinuousNoiseModel<T>& model)
                {
                        return block_diagonal<N>(core::continuous_white_noise<2, T>(dt, model.spectral_density));
                },
                [&](const DiscreteNoiseModel<T>& model)
                {
                        const T dt_2 = power<2>(dt) / 2;

                        const numerical::Matrix<2 * N, N, T> noise_transition =
                                block_diagonal<N>(numerical::Matrix<2, 1, T>{{dt_2}, {dt}});
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
numerical::Vector<N / 2, T> position_h(const numerical::Vector<N, T>& x)
{
        static_assert(N % 2 == 0);
        // px = px
        // py = py
        numerical::Vector<N / 2, T> res;
        for (std::size_t i = 0; i < N / 2; ++i)
        {
                res[i] = x[2 * i];
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Matrix<N / 2, N, T> position_hj(const numerical::Vector<N, T>& /*x*/)
{
        static_assert(N % 2 == 0);
        // px = px
        // py = py
        // Jacobian
        numerical::Matrix<N / 2, N, T> res(0);
        for (std::size_t i = 0; i < N / 2; ++i)
        {
                res[i, 2 * i] = 1;
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Vector<N, T> position_residual(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b)
{
        return a - b;
}
}
