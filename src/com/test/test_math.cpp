/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <limits>

namespace ns
{
namespace
{
template <typename I, typename T>
struct TestCeilFloor final
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

template struct TestCeilFloor<int, float>;
template struct TestCeilFloor<int, double>;
template struct TestCeilFloor<int, long double>;
template struct TestCeilFloor<long long, float>;
template struct TestCeilFloor<long long, double>;
template struct TestCeilFloor<long long, long double>;

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

//

static_assert(absolute(2) == 2);
static_assert(absolute(-2) == 2);
static_assert(absolute(2u) == 2);
static_assert(absolute(0) == 0);

template <typename T>
struct TestAbsoluteFloatingPoint final
{
        static_assert(absolute(T{2}) == T{2});
        static_assert(absolute(T{-2}) == T{2});
        static_assert(absolute(T{0}) == T{0});
        static_assert(absolute(std::numeric_limits<T>::infinity()) == std::numeric_limits<T>::infinity());
        static_assert(absolute(-std::numeric_limits<T>::infinity()) == std::numeric_limits<T>::infinity());
};

template struct TestAbsoluteFloatingPoint<float>;
template struct TestAbsoluteFloatingPoint<double>;
template struct TestAbsoluteFloatingPoint<long double>;
}
}
