/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/test/test.h>

#include <type_traits>

namespace ns
{
namespace
{
static_assert(power<0>(10) == 1);
static_assert(power<0>(-10) == 1);
static_assert(power<1>(10) == 10);
static_assert(power<1>(-10) == -10);
static_assert(power<2>(10) == 100);
static_assert(power<2>(-10) == 100);
static_assert(power<3>(10) == 1000);
static_assert(power<3>(-10) == -1000);
static_assert(power<4>(-10) == 10'000);
static_assert(power<5>(10) == 100'000);
static_assert(power<5>(-10) == -100'000);
static_assert(power<6>(10) == 1'000'000);
static_assert(power<6>(-10) == 1'000'000);
static_assert(power<7>(10) == 10'000'000);
static_assert(power<7>(-10) == -10'000'000);
static_assert(power<8>(10) == 100'000'000);
static_assert(power<8>(-10) == 100'000'000);
static_assert(power<9>(10) == 1'000'000'000);
static_assert(power<9>(-10) == -1'000'000'000);
static_assert(power<10>(10LL) == 10'000'000'000);
static_assert(power<10>(-10LL) == 10'000'000'000);
static_assert(power<10>(10.0) == 10'000'000'000);
static_assert(power<10>(-10.0) == 10'000'000'000);
static_assert(power<11>(10LL) == 100'000'000'000);
static_assert(power<11>(-10LL) == -100'000'000'000);
static_assert(power<11>(10.0) == 100'000'000'000);
static_assert(power<11>(-10.0) == -100'000'000'000);
static_assert(power<12>(10LL) == 1'000'000'000'000);
static_assert(power<13>(-10LL) == -10'000'000'000'000);
static_assert(power<14>(-10LL) == 100'000'000'000'000);
static_assert(power<15>(10LL) == 1'000'000'000'000'000);
static_assert(power<16>(10LL) == 10'000'000'000'000'000);
static_assert(power<17>(10LL) == 100'000'000'000'000'000);
static_assert(power<18>(10LL) == 1'000'000'000'000'000'000);
static_assert(power<19>(10ull) == 10'000'000'000'000'000'000u);

using S128 = signed __int128;
using F128 = __float128;
static_assert(power<20>(S128{10}) == 1 * square(square(S128{100'000})));
static_assert(power<20>(F128{10}) == 1 * square(square(S128{100'000})));
static_assert(power<20>(-F128{10}) == 1 * square(square(S128{100'000})));
static_assert(power<21>(S128{10}) == 10 * square(square(S128{100'000})));
static_assert(power<22>(S128{10}) == 100 * square(square(S128{100'000})));
static_assert(power<23>(S128{10}) == 1000 * square(square(S128{100'000})));
static_assert(power<23>(F128{10}) == 1000 * square(square(S128{100'000})));
static_assert(power<23>(-F128{10}) == -1000 * square(square(S128{100'000})));
static_assert(power<24>(S128{10}) == 10'000 * square(square(S128{100'000})));
static_assert(power<25>(S128{10}) == 100'000 * square(square(S128{100'000})));

template <typename T>
void compare(const T a, const T b, const T precision)
{
        if (a == b)
        {
                return;
        }

        const T abs = std::abs(a - b);
        if (!(abs <= precision))
        {
                error("abs error: " + to_string(a) + " is not equal to " + to_string(b));
        }

        const T rel = abs / std::max(std::abs(a), std::abs(b));
        if (!(rel <= precision))
        {
                error("rel error: " + to_string(a) + " is not equal to " + to_string(b));
        }
}

template <typename T>
void test_exponent(const std::type_identity_t<T>& precision)
{
        compare<T>(1, sqrt_s(T{1}), precision);
        compare<T>(1.41421356237309504876L, sqrt_s(T{2}), precision);
        compare<T>(2, sqrt_s(T{4}), precision);

        compare<T>(0, sqrt_s(T{-0.0001L}), precision);
        compare<T>(0.01L, sqrt_s(T{0.0001L}), precision);
}

void test()
{
        LOG("Test exponent");

        test_exponent<float>(0);
        test_exponent<double>(0);
        test_exponent<long double>(0);

        LOG("Test exponent passed");
}

TEST_SMALL("Exponent", test)
}
}
