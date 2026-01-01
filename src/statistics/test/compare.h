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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <cstddef>

namespace ns::statistics::test
{
template <typename T>
void compare(const T a, const T b)
{
        if (!(a == b))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <typename T>
        requires (std::is_floating_point_v<T>)
void compare(const T a, const T b, const T precision)
{
        if (!(std::abs(a - b) <= precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <std::size_t N, typename T>
void compare(
        const numerical::Vector<N, T>& a,
        const numerical::Vector<N, T>& b,
        const numerical::Vector<N, T>& precision)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(std::abs(a[i] - b[i]) <= precision[i]))
                {
                        error(to_string(a) + " is not equal to " + to_string(b));
                }
        }
}
}
