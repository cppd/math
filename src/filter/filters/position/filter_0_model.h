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
#include <src/com/variant.h>
#include <src/filter/core/kinematic_models.h>
#include <src/filter/filters/noise_model.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::filter::filters::position::filter_0_model
{
template <std::size_t N, typename T>
numerical::Vector<N, T> x(const numerical::Vector<N, T>& position)
{
        ASSERT(is_finite(position));

        return position;
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> p(const numerical::Vector<N, T>& position_variance)
{
        ASSERT(is_finite(position_variance));

        numerical::Matrix<N, N, T> res(numerical::ZERO_MATRIX);
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i, i] = position_variance[i];
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Vector<N, T> add_x(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b)
{
        return a + b;
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> f(const T /*dt*/)
{
        return block_diagonal<N>(numerical::Matrix<1, 1, T>{
                {1},
        });
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> q(const T dt, const NoiseModel<T>& noise_model)
{
        const auto visitors = Visitors{
                [&](const ContinuousNoiseModel<T>& model)
                {
                        return block_diagonal<N>(core::continuous_white_noise<1, T>(dt, model.spectral_density));
                },
                [&](const DiscreteNoiseModel<T>& model)
                {
                        const numerical::Matrix<N, N, T> noise_transition =
                                block_diagonal<N>(numerical::Matrix<1, 1, T>{{dt}});
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
numerical::Vector<N, T> position_h(const numerical::Vector<N, T>& x)
{
        // px = px
        return x;
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> position_hj(const numerical::Vector<N, T>& /*x*/)
{
        // px = px
        // py = py
        // Jacobian
        numerical::Matrix<N, N, T> res(numerical::ZERO_MATRIX);
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i, i] = 1;
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Vector<N, T> position_residual(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b)
{
        return a - b;
}
}
