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

#include "init_utility.h"

#include "quaternion.h"

#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <cmath>

namespace ns::filter::attitude::kalman
{
namespace
{
template <typename T>
numerical::Vector<3, T> orthogonal(const numerical::Vector<3, T>& v)
{
        const T x = std::abs(v[0]);
        const T y = std::abs(v[1]);
        const T z = std::abs(v[2]);

        if (x < y && x < z)
        {
                return {0, v[2], -v[1]};
        }

        if (y < z)
        {
                return {v[2], 0, -v[0]};
        }

        return {v[1], -v[0], 0};
}
}

template <typename T>
Quaternion<T> initial_quaternion(const numerical::Vector<3, T>& acc)
{
        const numerical::Vector<3, T> z = acc.normalized();
        const numerical::Vector<3, T> x = orthogonal(z).normalized();
        const numerical::Vector<3, T> y = cross(z, x).normalized();

        const numerical::Matrix<3, 3, T> rotation_matrix({x, y, z});
        const numerical::Quaternion<T> q =
                numerical::rotation_matrix_to_quaternion<numerical::Quaternion<T>>(rotation_matrix);

        return Quaternion<T>(q);
}

template <typename T>
Quaternion<T> initial_quaternion(const numerical::Vector<3, T>& acc, const numerical::Vector<3, T>& mag)
{
        const numerical::Vector<3, T> z = acc.normalized();
        const numerical::Vector<3, T> x = cross(mag, z).normalized();
        const numerical::Vector<3, T> y = cross(z, x).normalized();

        const numerical::Matrix<3, 3, T> rotation_matrix({x, y, z});
        const numerical::Quaternion<T> q =
                numerical::rotation_matrix_to_quaternion<numerical::Quaternion<T>>(rotation_matrix);

        return Quaternion<T>(q);
}

#define TEMPLATE(T)                                                                \
        template Quaternion<T> initial_quaternion(const numerical::Vector<3, T>&); \
        template Quaternion<T> initial_quaternion(const numerical::Vector<3, T>&, const numerical::Vector<3, T>&);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
