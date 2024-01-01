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

#include "constant.h"

#include <cmath>
#include <type_traits>

namespace ns
{
template <typename T>
[[nodiscard]] T normalize_angle(const T difference)
{
        return std::remainder(difference, 2 * PI<T>);
}

template <typename T, typename Previous>
[[nodiscard]] T unbound_angle(const Previous previous, const T next)
{
        const auto unbound = [&](const auto p)
        {
                static_assert(std::is_same_v<T, std::remove_cvref_t<decltype(p)>>);
                return p + normalize_angle(next - p);
        };

        if constexpr (requires { unbound(*previous); })
        {
                if (previous)
                {
                        return unbound(*previous);
                }
                return next;
        }
        else
        {
                return unbound(previous);
        }
}
}
