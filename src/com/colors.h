/*
Copyright (C) 2017 Topological Manifold

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

#include "com/vec.h"

#include <algorithm>
#include <array>
#include <cmath>

inline double rgb_to_srgb(double c)
{
        if (c > 1.0)
        {
                return 1.0;
        }
        if (c >= 0.0031308)
        {
                return 1.055 * std::pow(c, 1.0 / 2.4) - 0.055;
        }
        if (c >= 0.0)
        {
                return c * 12.92;
        }
        return 0.0;
}

inline double srgb_to_rgb(double c)
{
        if (c > 1.0)
        {
                return 1.0;
        }
        if (c >= 0.04045)
        {
                return std::pow((c + 0.055) / 1.055, 2.4);
        }
        if (c >= 0.0)
        {
                return c / 12.92;
        }
        return 0.0;
}

inline unsigned char rgb_float_to_srgb_int8(double c)
{
        return static_cast<unsigned char>(rgb_to_srgb(std::clamp(c, 0.0, 1.0)) * 255.0 + 0.5);
}

inline std::array<unsigned char, 3> rgb_float_to_srgb_int8(const vec3& c)
{
        return {{rgb_float_to_srgb_int8(c[0]), rgb_float_to_srgb_int8(c[1]), rgb_float_to_srgb_int8(c[2])}};
}

inline double srgb_int8_to_rgb_float(unsigned char c)
{
        return srgb_to_rgb(c / 255.0);
}

inline vec3 srgb_to_rgb(double r, double g, double b)
{
        return vec3(srgb_to_rgb(r), srgb_to_rgb(g), srgb_to_rgb(b));
}

inline vec3 srgb_to_rgb(const vec3& c)
{
        return srgb_to_rgb(c[0], c[1], c[2]);
}

inline double luminosity_rgb(const vec3& v)
{
        return 0.2126 * v[0] + 0.7152 * v[1] + 0.0722 * v[2];
}
