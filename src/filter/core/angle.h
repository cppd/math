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

#include <cmath>

namespace ns::filter::core
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
}
