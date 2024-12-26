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

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

namespace ns::filter::attitude::ekf
{
template <typename T>
[[nodiscard]] numerical::Matrix<3, 3, T> state_transition_matrix_3(const numerical::Vector<3, T>& w, T dt);

template <typename T>
[[nodiscard]] numerical::Matrix<6, 6, T> state_transition_matrix_6(const numerical::Vector<3, T>& w, T dt);

template <typename T>
[[nodiscard]] numerical::Matrix<3, 3, T> noise_covariance_matrix_3(T vr, T dt);

template <typename T>
[[nodiscard]] numerical::Matrix<6, 6, T> noise_covariance_matrix_6(const numerical::Vector<3, T>& w, T vr, T vw, T dt);
}
