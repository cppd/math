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
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

7.4.1 Hammersley and Halton sequences
*/

#pragma once

#include "type/number.h"

#include <algorithm>
#include <type_traits>

namespace ns
{
template <unsigned BASE, typename Result>
constexpr Result radical_inverse(unsigned long long v)
{
        static_assert(BASE >= 2);
        static_assert(std::is_floating_point_v<Result>);

        constexpr Result BASE_FLOAT = BASE;

        unsigned long long reverse = 0;
        Result base = 1;
        while (v)
        {
                const unsigned long long next = v / BASE;
                const unsigned long long digit = v - next * BASE;
                v = next;
                reverse = reverse * BASE + digit;
                base *= BASE_FLOAT;
        }
        return std::min(reverse / base, PREVIOUS_BEFORE_ONE<Result>);
}
}
