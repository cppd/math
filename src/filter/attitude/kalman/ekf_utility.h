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

#include "quaternion.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <cstddef>

namespace ns::filter::attitude::kalman
{
template <std::size_t N, typename T>
[[nodiscard]] numerical::Matrix<N, N, T> add_diagonal(numerical::Matrix<N, N, T> m, const numerical::Vector<N, T>& v)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                m[i, i] += v[i];
        }
        return m;
}

template <std::size_t R, std::size_t C, typename T>
[[nodiscard]] numerical::Matrix<R, C, T> mul_diagonal(
        const numerical::Matrix<R, C, T>& m,
        const numerical::Vector<C, T>& v)
{
        numerical::Matrix<R, C, T> res;
        for (std::size_t r = 0; r < R; ++r)
        {
                for (std::size_t c = 0; c < C; ++c)
                {
                        res[r, c] = m[r, c] * v[c];
                }
        }
        return res;
}

template <typename T>
[[nodiscard]] Quaternion<T> ekf_delta_quaternion(const numerical::Vector<3, T>& v)
{
        const T n2 = v.norm_squared();
        if (n2 <= 1)
        {
                return Quaternion<T>(std::sqrt(1 - n2), v);
        }
        return Quaternion<T>(1, v) / std::sqrt(1 + n2);
}
}
