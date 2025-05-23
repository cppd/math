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

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/variant.h>
#include <src/filter/core/kinematic_models.h>
#include <src/filter/filters/noise_model.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::filter::filters::speed::filter_1_model
{
template <std::size_t N, typename T>
numerical::Vector<N, T> x(const numerical::Vector<N, T>& position_velocity)
{
        ASSERT(is_finite(position_velocity));

        return position_velocity;
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> p(const numerical::Matrix<N, N, T>& position_velocity_p)
{
        ASSERT(is_finite(position_velocity_p));

        return position_velocity_p;
}

template <std::size_t N, typename T>
numerical::Vector<2 * N, T> add_x(const numerical::Vector<2 * N, T>& a, const numerical::Vector<2 * N, T>& b)
{
        return a + b;
}

template <std::size_t N, typename T>
numerical::Vector<2 * N, T> f(const T dt, const numerical::Vector<2 * N, T>& x)
{
        numerical::Vector<2 * N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 2 * i;
                const T p = x[b + 0];
                const T v = x[b + 1];
                res[b + 0] = p + dt * v;
                res[b + 1] = v;
        }
        return res;
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
}
