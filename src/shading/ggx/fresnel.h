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

/*
Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
Real-Time Rendering. Fourth Edition.
CRC Press, 2018.

9.5 Fresnel Reflectance
*/

#pragma once

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/exponent.h>

#include <cstddef>

namespace ns::shading::ggx
{
// (9.16)
// Schlick approximation of Fresnel reflectance
template <typename T, typename Color>
Color fresnel(const Color& f0, const T h_l)
{
        ASSERT(h_l >= 0);
        static constexpr Color WHITE(1);
        return interpolation(f0, WHITE, power<5>(1 - h_l));
}

// Integrate[Power[Sin[x],n-2]*Cos[x],{x,0,Pi/2},Assumptions->n\[Element]Integers&&n>=2]
// 1 / (n - 1)
// Simplify[(n-1)*Integrate[Power[Sin[x],n-2]*Cos[x]*(f0+(1-f0)*Power[1-Cos[x],5]),{x,0,Pi/2}]]
template <std::size_t N, typename Color>
constexpr Color fresnel_cosine_weighted_average(const Color& f0)
{
        using T = Color::DataType;

        if constexpr (N == 3)
        {
                return f0 * (T{20} / 21) + Color(T{1} / 21);
        }
        else if constexpr (N == 4)
        {
                return f0 * (495 * PI<T> / 256 - T{36} / 7) + Color(T{43} / 7 - 495 * PI<T> / 256);
        }
        else if constexpr (N == 5)
        {
                return f0 * (T{115} / 126) + Color(T{11} / 126);
        }
        else if constexpr (N == 6)
        {
                return f0 * (715 * PI<T> / 512 - T{220} / 63) + Color(T{283} / 63 - 715 * PI<T> / 512);
        }
        else if constexpr (N == 7)
        {
                return f0 * (T{29} / 33) + Color(T{4} / 33);
        }
        else if constexpr (N == 8)
        {
                return f0 * (2275 * PI<T> / 2048 - T{260} / 99) + Color(T{359} / 99 - 2275 * PI<T> / 2048);
        }
        else if constexpr (N == 9)
        {
                return f0 * (T{1093} / 1287) + Color(T{194} / 1287);
        }
        else
        {
                static_assert(false);
        }
}
}
