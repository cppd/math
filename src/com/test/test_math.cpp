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

#include <src/com/math.h>

namespace ns
{
namespace
{
template <typename I, typename T>
struct Test final
{
        static_assert(integral_floor<I, T>(-2) == -2);
        static_assert(integral_floor<I, T>(-1.5) == -2);
        static_assert(integral_floor<I, T>(-1) == -1);
        static_assert(integral_floor<I, T>(-0.5) == -1);
        static_assert(integral_floor<I, T>(-0) == 0);
        static_assert(integral_floor<I, T>(0) == 0);
        static_assert(integral_floor<I, T>(0.5) == 0);
        static_assert(integral_floor<I, T>(1.0) == 1);
        static_assert(integral_floor<I, T>(1.5) == 1);
        static_assert(integral_floor<I, T>(2) == 2);

        static_assert(integral_ceil<I, T>(-2) == -2);
        static_assert(integral_ceil<I, T>(-1.5) == -1);
        static_assert(integral_ceil<I, T>(-1) == -1);
        static_assert(integral_ceil<I, T>(-0.5) == 0);
        static_assert(integral_ceil<I, T>(-0) == 0);
        static_assert(integral_ceil<I, T>(0) == 0);
        static_assert(integral_ceil<I, T>(0.5) == 1);
        static_assert(integral_ceil<I, T>(1.0) == 1);
        static_assert(integral_ceil<I, T>(1.5) == 2);
        static_assert(integral_ceil<I, T>(2) == 2);
};

template struct Test<int, float>;
template struct Test<int, double>;
template struct Test<int, long double>;
template struct Test<long long, float>;
template struct Test<long long, double>;
template struct Test<long long, long double>;

//

template <typename T>
struct TestRoundUp final
{
        static_assert(round_up<T>(1, 1) == 1);
        static_assert(round_up<T>(1, 4) == 4);
        static_assert(round_up<T>(1, 11) == 11);
        static_assert(round_up<T>(10, 1) == 10);
        static_assert(round_up<T>(10, 4) == 12);
        static_assert(round_up<T>(10, 11) == 11);
        static_assert(round_up<T>(111, 1) == 111);
        static_assert(round_up<T>(111, 4) == 112);
        static_assert(round_up<T>(111, 11) == 121);
};

template struct TestRoundUp<unsigned>;
template struct TestRoundUp<unsigned long long>;
}
}
