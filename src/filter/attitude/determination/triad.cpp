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

#include "triad.h"

#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/rotation.h>
#include <src/numerical/vector.h>

#include <array>

namespace ns::filter::attitude::determination
{
template <typename T>
[[nodiscard]] numerical::QuaternionHJ<T, true> triad_attitude(
        const std::array<numerical::Vector<3, T>, 2>& observations,
        const std::array<numerical::Vector<3, T>, 2>& references)
{
        const auto rows = [](const std::array<numerical::Vector<3, T>, 2>& m)
        {
                numerical::Matrix<3, 3, T> res;
                res.row(0) = m[0].normalized();
                res.row(1) = cross(m[0], m[1]).normalized();
                res.row(2) = cross(m[0], res.row(1)).normalized();
                return res;
        };

        const numerical::Matrix<3, 3, T> s = rows(observations);
        const numerical::Matrix<3, 3, T> r = rows(references);

        const numerical::Matrix<3, 3, T> attitude = s.transposed() * r;

        return numerical::rotation_matrix_to_quaternion<numerical::QuaternionHJ<T, true>>(attitude);
}

#define TEMPLATE(T)                                               \
        template numerical::QuaternionHJ<T, true> triad_attitude( \
                const std::array<numerical::Vector<3, T>, 2>&, const std::array<numerical::Vector<3, T>, 2>&);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
