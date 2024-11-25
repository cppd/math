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

#include "constant.h"

#include <src/com/exponent.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::filter::attitude::ekf
{
namespace matrix_implementation
{
// cross(a, b) = cross_matrix_1(a) * b
template <typename T>
numerical::Matrix<3, 3, T> cross_matrix_1(const numerical::Vector<3, T>& v)
{
        return {
                {    0, -v[2],  v[1]},
                { v[2],     0, -v[0]},
                {-v[1],  v[0],     0}
        };
}

template <typename T>
numerical::Matrix<3, 3, T> cross_matrix_2(const numerical::Vector<3, T>& v)
{
        const T v00 = v[0] * v[0];
        const T v01 = v[0] * v[1];
        const T v02 = v[0] * v[2];
        const T v11 = v[1] * v[1];
        const T v12 = v[1] * v[2];
        const T v22 = v[2] * v[2];
        return {
                {-v11 - v22,        v01,        v02},
                {       v01, -v00 - v22,        v12},
                {       v02,        v12, -v00 - v11}
        };
}
}

template <std::size_t N, typename T>
numerical::Matrix<3, 3, T> cross_matrix(const numerical::Vector<3, T>& v)
{
        namespace impl = matrix_implementation;

        if constexpr (N == 1)
        {
                return impl::cross_matrix_1(v);
        }
        else if constexpr (N == 2)
        {
                return impl::cross_matrix_2(v);
        }
        else if constexpr (N == 3)
        {
                return impl::cross_matrix_1(-dot(v, v) * v);
        }
        else if constexpr (N == 4)
        {
                return -dot(v, v) * impl::cross_matrix_2(v);
        }
        else if constexpr (N == 5)
        {
                return impl::cross_matrix_1(square(dot(v, v)) * v);
        }
        else if constexpr (N == 6)
        {
                return impl::cross_matrix_2(dot(v, v) * v);
        }
        else
        {
                static_assert(false);
        }
}

template <typename T>
numerical::Matrix<3, 3, T> state_transition_theta_matrix(const numerical::Vector<3, T>& w, const T dt)
{
        const T n2 = w.norm_squared();
        const T n = std::sqrt(n2);

        if (n < W_THRESHOLD<T>)
        {
                const numerical::Matrix<3, 3, T> k0 = dt * cross_matrix<1>(w);
                const numerical::Matrix<3, 3, T> k1 = (dt * dt / 2) * cross_matrix<2>(w);
                return numerical::IDENTITY_MATRIX<3, T> - k0 + k1;
        }

        const numerical::Matrix<3, 3, T> k0 = (std::sin(n * dt) / n) * cross_matrix<1>(w);
        const numerical::Matrix<3, 3, T> k1 = ((1 - std::cos(n * dt)) / n2) * cross_matrix<2>(w);

        return numerical::IDENTITY_MATRIX<3, T> - k0 + k1;
}
}
