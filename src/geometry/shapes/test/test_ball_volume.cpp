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

#include "compare.h"

#include "../ball_volume.h"

#include <src/com/constant.h>
#include <src/com/exponent.h>

namespace ns::geometry::shapes::test
{
namespace
{
template <unsigned N>
constexpr long double PI_POW = power<N>(PI<long double>);

static_assert(compare(1, ball_volume<2>(), PI_POW<1>));
static_assert(compare(1, ball_volume<3>(), 4 * PI_POW<1> / 3));
static_assert(compare(1, ball_volume<4>(), PI_POW<2> / 2));
static_assert(compare(2, ball_volume<5>(), 8 * PI_POW<2> / 15));
static_assert(compare(1, ball_volume<6>(), PI_POW<3> / 6));
static_assert(compare(1, ball_volume<7>(), 16 * PI_POW<3> / 105));
static_assert(compare(2, ball_volume<8>(), PI_POW<4> / 24));
static_assert(compare(1, ball_volume<9>(), 32 * PI_POW<4> / 945));
static_assert(compare(2, ball_volume<10>(), PI_POW<5> / 120));
static_assert(compare(3, ball_volume<15>(), 256 * PI_POW<7> / 2027025));
static_assert(compare(1, ball_volume<20>(), PI_POW<10> / 3628800));
static_assert(compare(2, ball_volume<25>(), 8192 * PI_POW<12> / 7905853580625));
static_assert(compare(3, ball_volume<30>(), PI_POW<15> / 1307674368000));

static_assert(compare<float>(1, ball_volume<10, float>(5), power<10>(5.0L) * PI_POW<5> / 120));
static_assert(compare<double>(2, ball_volume<10, double>(5), power<10>(5.0L) * PI_POW<5> / 120));
static_assert(compare<long double>(2, ball_volume<10, long double>(5), power<10>(5.0L) * PI_POW<5> / 120));
}
}
