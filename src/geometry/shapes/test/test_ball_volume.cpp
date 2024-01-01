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

template <typename T>
struct Test final
{
        template <unsigned N>
        static constexpr bool cmp(const int epsilon_count, const T v)
        {
                return compare(epsilon_count, BALL_VOLUME<N, T>, v);
        }

        static_assert(cmp<2>(1, PI_POW<1>));
        static_assert(cmp<3>(1, 4 * PI_POW<1> / 3));
        static_assert(cmp<4>(1, PI_POW<2> / 2));
        static_assert(cmp<5>(2, 8 * PI_POW<2> / 15));
        static_assert(cmp<6>(1, PI_POW<3> / 6));
        static_assert(cmp<7>(1, 16 * PI_POW<3> / 105));
        static_assert(cmp<8>(2, PI_POW<4> / 24));
        static_assert(cmp<9>(1, 32 * PI_POW<4> / 945));
        static_assert(cmp<10>(2, PI_POW<5> / 120));
        static_assert(cmp<15>(3, 256 * PI_POW<7> / 2027025));
        static_assert(cmp<20>(1, PI_POW<10> / 3628800));
        static_assert(cmp<25>(2, 8192 * PI_POW<12> / 7905853580625));
        static_assert(cmp<30>(3, PI_POW<15> / 1307674368000));

        static_assert(compare<T>(1, ball_volume<5, T>(0.5), power<5>(0.5L) * BALL_VOLUME<5, long double>));
        static_assert(compare<T>(2, ball_volume<5, T>(5), power<5>(5.0L) * BALL_VOLUME<5, long double>));
        static_assert(compare<T>(1, ball_volume<10, T>(0.5), power<10>(0.5L) * BALL_VOLUME<10, long double>));
        static_assert(compare<T>(2, ball_volume<10, T>(5), power<10>(5.0L) * BALL_VOLUME<10, long double>));
};

template struct Test<float>;
template struct Test<double>;
template struct Test<long double>;
}
}
