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

#include "conversion.h"

#include <src/numerical/vector.h>

namespace ns::color
{
class RGB8 final
{
        unsigned char red_;
        unsigned char green_;
        unsigned char blue_;

public:
        constexpr RGB8(const unsigned char red, const unsigned char green, const unsigned char blue)
                : red_(red),
                  green_(green),
                  blue_(blue)
        {
        }

        [[nodiscard]] constexpr bool operator==(const RGB8 v) const
        {
                return red_ == v.red_ && green_ == v.green_ && blue_ == v.blue_;
        }

        [[nodiscard]] constexpr unsigned char red() const
        {
                return red_;
        }

        [[nodiscard]] constexpr unsigned char green() const
        {
                return green_;
        }

        [[nodiscard]] constexpr unsigned char blue() const
        {
                return blue_;
        }

        [[nodiscard]] constexpr float linear_red() const
        {
                return color::srgb_uint8_to_linear_float(red_);
        }

        [[nodiscard]] constexpr float linear_green() const
        {
                return color::srgb_uint8_to_linear_float(green_);
        }

        [[nodiscard]] constexpr float linear_blue() const
        {
                return color::srgb_uint8_to_linear_float(blue_);
        }

        [[nodiscard]] constexpr numerical::Vector<3, float> linear_rgb() const
        {
                return {linear_red(), linear_green(), linear_blue()};
        }
};

[[nodiscard]] inline RGB8 make_rgb8(const float red, const float green, const float blue)
{
        const unsigned char r = color::linear_float_to_srgb_uint8(red);
        const unsigned char g = color::linear_float_to_srgb_uint8(green);
        const unsigned char b = color::linear_float_to_srgb_uint8(blue);
        return {r, g, b};
}

template <typename T>
[[nodiscard]] RGB8 make_rgb8(const numerical::Vector<3, T>& v)
{
        static_assert(std::is_floating_point_v<T>);
        return make_rgb8(v[0], v[1], v[2]);
}
}
