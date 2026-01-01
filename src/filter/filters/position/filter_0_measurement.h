/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::filter::filters::position::filter_0_measurement
{
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
