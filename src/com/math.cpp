/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "math.h"

namespace ns
{
using U128 = unsigned __int128;

static_assert(power<0>(10u) == 1);
static_assert(power<1>(10u) == 10);
static_assert(power<2>(10u) == 100);
static_assert(power<3>(10u) == 1000);
static_assert(power<4>(10u) == 10000);
static_assert(power<5>(10u) == 100000);
static_assert(power<6>(10u) == 1000000);
static_assert(power<7>(10u) == 10000000);
static_assert(power<8>(10u) == 100000000);
static_assert(power<9>(10u) == 1000000000);
static_assert(power<10>(10ull) == 10000000000);
static_assert(power<11>(10ull) == 100000000000);
static_assert(power<12>(10ull) == 1000000000000);
static_assert(power<13>(10ull) == 10000000000000);
static_assert(power<14>(10ull) == 100000000000000);
static_assert(power<15>(10ull) == 1000000000000000);
static_assert(power<16>(10ull) == 10000000000000000);
static_assert(power<17>(10ull) == 100000000000000000);
static_assert(power<18>(10ull) == 1000000000000000000);
static_assert(power<19>(10ull) == 10000000000000000000u);
static_assert(power<20>(U128(10)) == 1 * square(square(U128(100000))));
static_assert(power<21>(U128(10)) == 10 * square(square(U128(100000))));
static_assert(power<22>(U128(10)) == 100 * square(square(U128(100000))));
static_assert(power<23>(U128(10)) == 1000 * square(square(U128(100000))));
static_assert(power<24>(U128(10)) == 10000 * square(square(U128(100000))));
static_assert(power<25>(U128(10)) == 100000 * square(square(U128(100000))));
}
