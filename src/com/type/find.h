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

#include <gmpxx.h>
#include <type_traits>

namespace ns
{
// clang-format off
template<int BITS>
using LeastSignedInteger =
        std::conditional_t<BITS <=   7, std::int_least8_t,
        std::conditional_t<BITS <=  15, std::int_least16_t,
        std::conditional_t<BITS <=  31, std::int_least32_t,
        std::conditional_t<BITS <=  63, std::int_least64_t,
        std::conditional_t<BITS <= 127, signed __int128,
        mpz_class>>>>>;

template<int BITS>
using LeastUnsignedInteger =
        std::conditional_t<BITS <=   8, std::uint_least8_t,
        std::conditional_t<BITS <=  16, std::uint_least16_t,
        std::conditional_t<BITS <=  32, std::uint_least32_t,
        std::conditional_t<BITS <=  64, std::uint_least64_t,
        std::conditional_t<BITS <= 128, unsigned __int128,
        mpz_class>>>>>;
// clang-format on
}
