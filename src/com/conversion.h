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

#include "constant.h"

#include <cmath>

namespace ns
{
template <typename T>
[[nodiscard]] int points_to_pixels(const T points, const T pixels_per_inch)
{
        static_assert(std::is_floating_point_v<T>);

        return std::lround(points / 72 * pixels_per_inch);
}

template <typename T>
[[nodiscard]] int millimeters_to_pixels(const T millimeters, const T pixels_per_inch)
{
        static_assert(std::is_floating_point_v<T>);

        return std::lround(millimeters / T{25.4L} * pixels_per_inch);
}

template <typename T>
[[nodiscard]] constexpr T pixels_to_millimeters(const T pixels, const T pixels_per_inch)
{
        static_assert(std::is_floating_point_v<T>);

        return pixels / pixels_per_inch * T{25.4L};
}

template <typename T>
[[nodiscard]] constexpr T size_to_ppi(const T size_in_mm, const unsigned size_in_pixels)
{
        static_assert(std::is_floating_point_v<T>);

        return size_in_pixels / size_in_mm * T{25.4L};
}

template <typename T>
[[nodiscard]] constexpr T radians_to_degrees(const T angle)
{
        static_assert(std::is_floating_point_v<T>);

        return angle * (180 / PI<T>);
}

template <typename T>
[[nodiscard]] constexpr T degrees_to_radians(const T angle)
{
        static_assert(std::is_floating_point_v<T>);

        return angle * (PI<T> / 180);
}

template <typename T>
[[nodiscard]] constexpr T mps_to_kph(const T mps)
{
        static_assert(std::is_floating_point_v<T>);

        return mps * T{3.6L};
}

template <typename T>
[[nodiscard]] constexpr T kph_to_mps(const T kph)
{
        static_assert(std::is_floating_point_v<T>);

        return kph / T{3.6L};
}
}
