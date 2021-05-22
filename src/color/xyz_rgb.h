/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "xyz_versions.h"

#include <array>

namespace ns::color
{
template <XYZ xyz_version, typename T>
constexpr std::enable_if_t<xyz_version == XYZ_31, std::array<T, 3>> xyz_to_linear_srgb(T x, T y, T z)
{
        static_assert(std::is_floating_point_v<T>);

        std::array<T, 3> rgb;

        rgb[0] = T(+3.2406255) * x + T(-1.5372080) * y + T(-0.4986286) * z;
        rgb[1] = T(-0.9689307) * x + T(+1.8757561) * y + T(+0.0415175) * z;
        rgb[2] = T(+0.0557101) * x + T(-0.2040211) * y + T(+1.0569959) * z;

        return rgb;
}

template <XYZ xyz_version, typename T>
constexpr std::enable_if_t<xyz_version == XYZ_31, std::array<T, 3>> linear_srgb_to_xyz(T r, T g, T b)
{
        static_assert(std::is_floating_point_v<T>);

        std::array<T, 3> xyz;

        xyz[0] = T(0.4124) * r + T(0.3576) * g + T(0.1805) * b;
        xyz[1] = T(0.2126) * r + T(0.7152) * g + T(0.0722) * b;
        xyz[2] = T(0.0193) * r + T(0.1192) * g + T(0.9505) * b;

        return xyz;
}
}
