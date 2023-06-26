/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "constant.h"

#include <cmath>

namespace ns
{
namespace angle_implementation
{
template <typename T>
[[nodiscard]] T normalize_angle(const T difference)
{
        return std::remainder(difference, 2 * PI<T>);
}

template <typename T>
[[nodiscard]] T unbound_angle(const T previous, const T next)
{
        return previous + normalize_angle(next - previous);
}
}

template <typename T>
[[nodiscard]] T normalize_angle(const T difference)
{
        return angle_implementation::normalize_angle(difference);
}

template <typename T, typename Previous>
[[nodiscard]] T unbound_angle(const Previous previous, const T next)
{
        if constexpr (requires { angle_implementation::unbound_angle(*previous, next); })
        {
                if (previous)
                {
                        return angle_implementation::unbound_angle(*previous, next);
                }
                return next;
        }
        else
        {
                return angle_implementation::unbound_angle(previous, next);
        }
}
}
