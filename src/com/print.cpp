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

#include "print.h"

#if 0 && !defined(__clang__)
#include <array>
#include <quadmath.h>
#endif

namespace ns
{
#if 0 && !defined(__clang__)
std::string to_string(__float128 t)
{
        constexpr const char* QUAD_MATH_FORMAT = "%.36Qe"; // "%+-#*.36Qe"

        std::array<char, 1000> buf;
        quadmath_snprintf(buf.data(), buf.size(), QUAD_MATH_FORMAT, t);
        return buf.data();
}
#endif
}
