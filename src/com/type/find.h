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

#pragma once

#include <gmpxx.h>

#include <cstdint>
#include <type_traits>

namespace ns
{
// clang-format off
template<unsigned BIT_COUNT>
using LeastSignedInteger =
        std::conditional_t<BIT_COUNT <=   7, std::int_least8_t,
        std::conditional_t<BIT_COUNT <=  15, std::int_least16_t,
        std::conditional_t<BIT_COUNT <=  31, std::int_least32_t,
        std::conditional_t<BIT_COUNT <=  63, std::int_least64_t,
        std::conditional_t<BIT_COUNT <= 127, signed __int128,
        mpz_class>>>>>;

template<unsigned BIT_COUNT>
using LeastUnsignedInteger =
        std::conditional_t<BIT_COUNT <=   8, std::uint_least8_t,
        std::conditional_t<BIT_COUNT <=  16, std::uint_least16_t,
        std::conditional_t<BIT_COUNT <=  32, std::uint_least32_t,
        std::conditional_t<BIT_COUNT <=  64, std::uint_least64_t,
        std::conditional_t<BIT_COUNT <= 128, unsigned __int128,
        mpz_class>>>>>;
// clang-format on
}
