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
inline int points_to_pixels(const double points, const double pixels_per_inch)
{
        return std::lround(points / 72.0 * pixels_per_inch);
}

inline int millimeters_to_pixels(const double millimeters, const double pixels_per_inch)
{
        return std::lround(millimeters / 25.4 * pixels_per_inch);
}

constexpr double pixels_to_millimeters(const double pixels, const double pixels_per_inch)
{
        return pixels / pixels_per_inch * 25.4;
}

constexpr double size_to_ppi(const double size_in_mm, const unsigned size_in_pixels)
{
        return size_in_pixels / size_in_mm * 25.4;
}

template <typename T>
constexpr T radians_to_degrees(const T angle)
{
        static_assert(std::is_floating_point_v<T>);

        return angle * (180 / PI<T>);
}

template <typename T>
constexpr T degrees_to_radians(const T angle)
{
        static_assert(std::is_floating_point_v<T>);

        return angle * (PI<T> / 180);
}
}
