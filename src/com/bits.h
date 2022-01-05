/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "error.h"
#include "print.h"

#include "type/concept.h"

namespace ns
{
template <typename T>
constexpr T bit_width(T n)
{
        static_assert(Integral<T>);

        if (n <= 0)
        {
                error("Bit width parameter " + to_string(n) + " is not positive");
        }

        T b = 1;
        while (n >>= 1)
        {
                ++b;
        }
        return b;
}
}
