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
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <cmath>

namespace ns::filter::attitude::ekf
{
template <typename T>
numerical::Vector<3, T> orthogonal(const numerical::Vector<3, T>& v)
{
        const T x = std::abs(v[0]);
        const T y = std::abs(v[1]);
        const T z = std::abs(v[2]);
        numerical::Vector<3, T> res;
        if (x < y && x < z)
        {
                res = {0, v[2], -v[1]};
        }
        else if (y < z)
        {
                res = {v[2], 0, -v[0]};
        }
        else
        {
                res = {v[1], -v[0], 0};
        }
        return res.normalized();
}

template <typename T>
numerical::Quaternion<T> initial_quaternion(const numerical::Vector<3, T>& acc)
{
        const numerical::Vector<3, T> z = acc.normalized();
        const numerical::Vector<3, T> x = orthogonal(z);
        const numerical::Vector<3, T> y = cross(z, x);
        const numerical::Matrix<3, 3, T> rotation_matrix({x, y, z});
        return numerical::rotation_matrix_to_unit_quaternion(rotation_matrix);
}

template <typename T>
numerical::Quaternion<T> initial_quaternion(const numerical::Vector<3, T>& acc, const numerical::Vector<3, T>& mag)
{
        const numerical::Vector<3, T> z = acc.normalized();
        const numerical::Vector<3, T> x = cross(mag, z).normalized();
        const numerical::Vector<3, T> y = cross(z, x);
        const numerical::Matrix<3, 3, T> rotation_matrix({x, y, z});
        return numerical::rotation_matrix_to_unit_quaternion(rotation_matrix);
}
}
