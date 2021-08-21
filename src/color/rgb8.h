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

#include "conversion.h"

#include <src/numerical/vec.h>

namespace ns
{
struct RGB8 final
{
        unsigned char red;
        unsigned char green;
        unsigned char blue;

        constexpr RGB8(unsigned char red, unsigned char green, unsigned char blue) : red(red), green(green), blue(blue)
        {
        }

        constexpr bool operator==(const RGB8& v) const
        {
                return red == v.red && green == v.green && blue == v.blue;
        }

        constexpr float linear_red() const
        {
                return color::srgb_uint8_to_linear_float(red);
        }

        constexpr float linear_green() const
        {
                return color::srgb_uint8_to_linear_float(green);
        }

        constexpr float linear_blue() const
        {
                return color::srgb_uint8_to_linear_float(blue);
        }

        constexpr Vector<3, float> linear_rgb() const
        {
                return Vector<3, float>(linear_red(), linear_green(), linear_blue());
        }
};

inline RGB8 make_rgb8(float red, float green, float blue)
{
        unsigned char r = color::linear_float_to_srgb_uint8(red);
        unsigned char g = color::linear_float_to_srgb_uint8(green);
        unsigned char b = color::linear_float_to_srgb_uint8(blue);
        return RGB8(r, g, b);
}

template <typename T>
RGB8 make_rgb8(const Vector<3, T>& v)
{
        static_assert(std::is_floating_point_v<T>);
        return make_rgb8(v[0], v[1], v[2]);
}
}
