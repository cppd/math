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

#include <src/model/mesh.h>
#include <src/numerical/matrix.h>
#include <src/numerical/transform.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::model::mesh
{
template <std::size_t N>
numerical::Matrix<N + 1, N + 1, double> model_matrix_for_size_and_position(
        const Mesh<N>& mesh,
        const double size,
        const numerical::Vector<N, double>& position)
{
        const numerical::Matrix<N + 1, N + 1, double> t1 =
                numerical::transform::translate(to_vector<double>(-mesh.center));
        const numerical::Matrix<N + 1, N + 1, double> t2 =
                numerical::transform::scale(numerical::Vector<N, double>(size / mesh.length));
        const numerical::Matrix<N + 1, N + 1, double> t3 = numerical::transform::translate(position);
        return t3 * t2 * t1;
}
}
