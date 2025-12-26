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

#include <src/com/constant.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cmath>

namespace ns::filter::filters::com
{
template <typename T>
[[nodiscard]] T wrap_angle(const T angle)
{
        return std::remainder(angle, 2 * PI<T>);
}

template <typename T>
[[nodiscard]] T unwrap_angle(const T reference, const T angle)
{
        return reference + wrap_angle(angle - reference);
}

template <typename T>
[[nodiscard]] T angle(const T x, const T y)
{
        return std::atan2(y, x);
}

template <typename T>
[[nodiscard]] numerical::Vector<2, T> angle_jacobian(const T x, const T y)
{
        const T d = x * x + y * y;
        return {-y / d, x / d};
}

template <typename T>
[[nodiscard]] numerical::Vector<2, T> angle_vector(const T angle)
{
        return {std::cos(angle), std::sin(angle)};
}

template <typename T>
[[nodiscard]] numerical::Vector<2, T> rotate(const numerical::Vector<2, T>& v, const T angle)
{
        const T x = v[0];
        const T y = v[1];
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {
                x * cos - y * sin,
                x * sin + y * cos,
        };
}

template <typename T>
[[nodiscard]] numerical::Matrix<2, 3, T> rotate_jacobian(const numerical::Vector<2, T>& v, const T angle)
{
        const T x = v[0];
        const T y = v[1];
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {
                {cos, -sin, -x * sin - y * cos},
                {sin,  cos,  x * cos - y * sin},
        };
}
}
