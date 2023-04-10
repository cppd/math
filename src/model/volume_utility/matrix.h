/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "position.h"

#include "../volume.h"

#include <src/numerical/matrix.h>
#include <src/numerical/transform.h>
#include <src/numerical/vector.h>

namespace ns::model::volume
{
template <std::size_t N>
Matrix<N + 1, N + 1, double> matrix_for_image_size(const std::array<int, N>& size)
{
        const double max_size = *std::max_element(size.cbegin(), size.cend());
        Matrix<N + 1, N + 1, double> matrix = IDENTITY_MATRIX<N + 1, double>;
        for (std::size_t i = 0; i < N; ++i)
        {
                matrix(i, i) = size[i] / max_size;
        }
        return matrix;
}

template <std::size_t N>
Matrix<N + 1, N + 1, double> model_matrix_for_size_and_position(
        const Volume<N>& volume,
        const double size,
        const Vector<N, double>& position)
{
        const auto [center, length] = center_and_length(volume);
        const Matrix<N + 1, N + 1, double> t1 = numerical::transform::translate(-center);
        const Matrix<N + 1, N + 1, double> t2 = numerical::transform::scale(Vector<N, double>(size / length));
        const Matrix<N + 1, N + 1, double> t3 = numerical::transform::translate(position);
        return t3 * t2 * t1;
}
}
