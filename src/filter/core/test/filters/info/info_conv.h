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

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::core::test::filters::info::conv
{
template <typename T>
[[nodiscard]] T position(const numerical::Vector<2, T>& x)
{
        return x[0];
}

template <typename T>
[[nodiscard]] T position_p(const std::optional<numerical::Matrix<2, 2, T>>& p, const numerical::Matrix<2, 2, T>& i)
{
        if (p)
        {
                return (*p)[0, 0];
        }
        return i.inversed()[0, 0];
}

template <typename T>
[[nodiscard]] numerical::Vector<2, T> position_speed(const numerical::Vector<2, T>& x)
{
        return x;
}

template <typename T>
[[nodiscard]] numerical::Matrix<2, 2, T> position_speed_p(
        const std::optional<numerical::Matrix<2, 2, T>>& p,
        const numerical::Matrix<2, 2, T>& i)
{
        if (p)
        {
                return *p;
        }
        return i.inversed();
}

template <typename T>
[[nodiscard]] T speed(const numerical::Vector<2, T>& x)
{
        return x[1];
}

template <typename T>
[[nodiscard]] T speed_p(const std::optional<numerical::Matrix<2, 2, T>>& p, const numerical::Matrix<2, 2, T>& i)
{
        if (p)
        {
                return (*p)[1, 1];
        }
        return i.inversed()[1, 1];
}
}
