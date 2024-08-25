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

#include "noise_model.h"

#include <src/com/variant.h>
#include <src/filter/core/kinematic_models.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

namespace ns::filter::core::test::filters::ekf_model
{
template <typename T>
numerical::Vector<2, T> add_x(const numerical::Vector<2, T>& a, const numerical::Vector<2, T>& b)
{
        return a + b;
}

template <typename T>
numerical::Matrix<2, 2, T> f(const T dt)
{
        // x[0] = x[0] + dt * x[1]
        // x[1] = x[1]
        // Jacobian matrix
        //  1 dt
        //  0  1
        return {
                {1, dt},
                {0,  1}
        };
}

template <typename T>
numerical::Matrix<2, 2, T> q(const T dt, const NoiseModel<T>& noise_model)
{
        const auto visitors = Visitors{
                [&](const ContinuousNoiseModel<T>& model)
                {
                        return continuous_white_noise<2, T>(dt, model.spectral_density);
                },
                [&](const DiscreteNoiseModel<T>& model)
                {
                        const T dt_2 = power<2>(dt) / 2;
                        const numerical::Matrix<2, 1, T> noise_transition{{dt_2}, {dt}};
                        const numerical::Matrix<1, 1, T> covariance{{model.variance}};
                        return noise_transition * covariance * noise_transition.transposed();
                }};
        return std::visit(visitors, noise_model);
}

//

template <typename T>
numerical::Matrix<1, 1, T> position_r(const T position_variance)
{
        return {{position_variance}};
}

template <typename T>
numerical::Vector<1, T> position_h(const numerical::Vector<2, T>& x)
{
        // x = x[0]
        return numerical::Vector<1, T>(x[0]);
}

template <typename T>
numerical::Matrix<1, 2, T> position_hj(const numerical::Vector<2, T>& /*x*/)
{
        // x = x[0]
        // Jacobian matrix
        //  1 0
        return {
                {1, 0}
        };
}

template <typename T>
numerical::Vector<1, T> position_residual(const numerical::Vector<1, T>& a, const numerical::Vector<1, T>& b)
{
        return a - b;
}

//

template <typename T>
numerical::Matrix<2, 2, T> position_speed_r(const T position_variance, const T speed_variance)
{
        return {
                {position_variance,              0},
                {                0, speed_variance}
        };
}

template <typename T>
numerical::Vector<2, T> position_speed_h(const numerical::Vector<2, T>& x)
{
        // x = x[0]
        // v = x[1]
        return x;
}

template <typename T>
numerical::Matrix<2, 2, T> position_speed_hj(const numerical::Vector<2, T>& /*x*/)
{
        // x = x[0]
        // v = x[1]
        // Jacobian matrix
        //  1 0
        //  0 1
        return {
                {1, 0},
                {0, 1}
        };
}

template <typename T>
numerical::Vector<2, T> position_speed_residual(const numerical::Vector<2, T>& a, const numerical::Vector<2, T>& b)
{
        return a - b;
}

//

template <typename T>
numerical::Matrix<1, 1, T> speed_r(const T speed_variance)
{
        return {{speed_variance}};
}

template <typename T>
numerical::Vector<1, T> speed_h(const numerical::Vector<2, T>& x)
{
        // v = x[1]
        return numerical::Vector<1, T>(x[1]);
}

template <typename T>
numerical::Matrix<1, 2, T> speed_hj(const numerical::Vector<2, T>& /*x*/)
{
        // v = x[1]
        // Jacobian matrix
        //  0 1
        return {
                {0, 1},
        };
}

template <typename T>
numerical::Vector<1, T> speed_residual(const numerical::Vector<1, T>& a, const numerical::Vector<1, T>& b)
{
        return a - b;
}
}
