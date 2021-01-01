/*
Copyright (C) 2017-2021 Topological Manifold

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
#include <src/numerical/vec.h>

namespace ns::volume
{
template <std::size_t N>
Matrix<N + 1, N + 1, double> matrix_for_image_size(const std::array<int, N>& size)
{
        Matrix<N + 1, N + 1, double> matrix(1);
        double max_size = *std::max_element(size.cbegin(), size.cend());
        for (unsigned i = 0; i < N; ++i)
        {
                matrix(i, i) = size[i] / max_size;
        }
        return matrix;
}

template <std::size_t N>
Matrix<N + 1, N + 1, double> model_matrix_for_size_and_position(
        const Volume<N>& volume,
        double size,
        const Vector<N, double>& position)
{
        Vector<N, double> center;
        double length;
        std::tie(center, length) = center_and_length(volume);
        Matrix<N + 1, N + 1, double> t1 = matrix::translate(-center);
        Matrix<N + 1, N + 1, double> t2 = matrix::scale(Vector<N, double>(size / length));
        Matrix<N + 1, N + 1, double> t3 = matrix::translate(position);
        return t3 * t2 * t1;
}
}
