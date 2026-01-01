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

#pragma once

#include <src/filter/filters/com/angle.h>
#include <src/filter/filters/com/variance.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

namespace ns::filter::filters::acceleration::filter_0_conv
{
template <typename T>
[[nodiscard]] numerical::Vector<2, T> position(const numerical::Vector<8, T>& x)
{
        return {x[0], x[3]};
}

template <typename T>
[[nodiscard]] numerical::Matrix<2, 2, T> position_p(const numerical::Matrix<8, 8, T>& p)
{
        return {
                {p[0, 0], p[0, 3]},
                {p[3, 0], p[3, 3]}
        };
}

template <typename T>
[[nodiscard]] numerical::Vector<2, T> velocity(const numerical::Vector<8, T>& x)
{
        return {x[1], x[4]};
}

template <typename T>
[[nodiscard]] T velocity_angle(const numerical::Vector<8, T>& x)
{
        const T vx = x[1];
        const T vy = x[4];
        return com::angle(vx, vy);
}

template <typename T>
[[nodiscard]] numerical::Matrix<2, 2, T> velocity_p(const numerical::Matrix<8, 8, T>& p)
{
        return {
                {p[1, 1], p[1, 4]},
                {p[4, 1], p[4, 4]}
        };
}

template <typename T>
[[nodiscard]] T speed(const numerical::Vector<8, T>& x)
{
        return velocity(x).norm();
}

template <typename T>
[[nodiscard]] T speed_p(const numerical::Vector<8, T>& x, const numerical::Matrix<8, 8, T>& p)
{
        return com::compute_speed_p(velocity(x), velocity_p(p));
}

template <typename T>
[[nodiscard]] T angle(const numerical::Vector<8, T>& x)
{
        return x[6];
}

template <typename T>
[[nodiscard]] T angle_p(const numerical::Matrix<8, 8, T>& p)
{
        return p[6, 6];
}

template <typename T>
[[nodiscard]] T angle_r(const numerical::Vector<8, T>& x)
{
        return x[7];
}

template <typename T>
[[nodiscard]] T angle_r_p(const numerical::Matrix<8, 8, T>& p)
{
        return p[7, 7];
}
}
