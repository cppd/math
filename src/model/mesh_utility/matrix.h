/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "../mesh.h"

#include <src/numerical/matrix.h>
#include <src/numerical/transform.h>
#include <src/numerical/vector.h>

namespace ns::mesh
{
template <std::size_t N>
Matrix<N + 1, N + 1, double> model_matrix_for_size_and_position(
        const Mesh<N>& mesh,
        const double size,
        const Vector<N, double>& position)
{
        Matrix<N + 1, N + 1, double> t1 = matrix::translate(to_vector<double>(-mesh.center));
        Matrix<N + 1, N + 1, double> t2 = matrix::scale(Vector<N, double>(size / mesh.length));
        Matrix<N + 1, N + 1, double> t3 = matrix::translate(position);
        return t3 * t2 * t1;
}
}
