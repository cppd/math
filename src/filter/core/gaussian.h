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

#include <src/com/constant.h>
#include <src/com/exponent.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <cstddef>

namespace ns::filter::core
{
template <std::size_t N, typename T>
[[nodiscard]] T compute_mahalanobis_distance_squared(const Vector<N, T>& residual, const Matrix<N, N, T>& s_inversed)
{
        return dot(residual * s_inversed, residual);
}

template <std::size_t N, typename T>
[[nodiscard]] T compute_likelihood(const T mahalanobis_distance_squared, const Matrix<N, N, T>& s)
{
        const T numerator = std::exp(-mahalanobis_distance_squared / 2);
        const T denominator = std::sqrt(power<N>(2 * PI<T>) * std::abs(s.determinant()));
        return numerator / denominator;
}
}
