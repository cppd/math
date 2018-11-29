/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "types.h"

#include <cmath>
#include <limits>
#include <type_traits>
#if !defined(__clang__)
#include <quadmath.h>
#endif

template <typename T>
inline constexpr T PI = 3.1415926535897932384626433832795028841971693993751L;
inline constexpr const char PI_STR[] = "3.1415926535897932384626433832795028841971693993751";
template <typename T>
inline constexpr T TWO_PI = 6.2831853071795864769252867665590057683943387987502L;

template <typename T>
constexpr T square(const T& v)
{
        return v * v;
}

template <typename T>
T any_abs(T a)
{
        static_assert(std::is_floating_point_v<T>);
        return std::abs(a);
}
template <typename T>
T any_fma(T a, T b, T c)
{
        static_assert(std::is_floating_point_v<T>);
        return std::fma(a, b, c);
}
template <typename T>
T any_sqrt(T a)
{
        static_assert(std::is_floating_point_v<T>);
        return std::sqrt(a);
}
template <typename T>
T any_sin(T a)
{
        static_assert(std::is_floating_point_v<T>);
        return std::sin(a);
}
template <typename T>
T any_cos(T a)
{
        static_assert(std::is_floating_point_v<T>);
        return std::cos(a);
}

#if !defined(__clang__)
inline __float128 any_abs(__float128 a)
{
        return fabsq(a);
}
inline __float128 any_fma(__float128 a, __float128 b, __float128 c)
{
        return fmaq(a, b, c);
}
inline __float128 any_sqrt(__float128 a)
{
        return sqrtq(a);
}
inline __float128 any_sin(__float128 a)
{
        return sinq(a);
}
inline __float128 any_cos(__float128 a)
{
        return cosq(a);
}
#endif

template <typename T>
constexpr bool is_finite(T v)
{
        static_assert(std::is_floating_point_v<T> && std::numeric_limits<T>::is_specialized);
        // std::isfinite не работает с -Ofast (-ffast-math),
        // поэтому сравнение с std::numeric_limits<T> lowest и max
        return v >= std::numeric_limits<T>::lowest() && v <= std::numeric_limits<T>::max();
}

constexpr bool is_finite(__float128 v)
{
        // вместо finiteq и по аналогии с другими типами
        return v >= limits<__float128>::lowest() && v <= limits<__float128>::max();
}

template <typename T, typename F>
T interpolation(T v0, T v1, F x)
{
        static_assert(is_native_floating_point<T> && is_native_floating_point<F>);

        return any_fma(static_cast<T>(x), v1, any_fma(-static_cast<T>(x), v0, v0));
}

template <unsigned Exp, typename T>
constexpr T power([[maybe_unused]] T base)
{
        static_assert(is_native_integral<T> && is_unsigned<T>);

        if constexpr (Exp == 0)
        {
                return 1;
        }
        if constexpr (Exp == 1)
        {
                return base;
        }
        if constexpr (Exp == 2)
        {
                return base * base;
        }
        if constexpr (Exp == 3)
        {
                return base * base * base;
        }
        if constexpr (Exp == 4)
        {
                T t = base * base;
                return t * t;
        }
        if constexpr (Exp == 5)
        {
                T t = base * base;
                return t * t * base;
        }
        if constexpr (Exp == 6)
        {
                T t = base * base;
                return t * t * t;
        }
        if constexpr (Exp == 7)
        {
                T t = base * base;
                return t * t * t * base;
        }
        if constexpr (Exp == 8)
        {
                T t = base * base;
                T t2 = t * t;
                return t2 * t2;
        }
        if constexpr (Exp >= 9)
        {
                unsigned exp = Exp;

                T res = (exp & 1) ? base : 1;
                exp >>= 1;
                while (exp)
                {
                        base *= base;
                        if (exp & 1)
                        {
                                res *= base;
                        }
                        exp >>= 1;
                }

                return res;
        }
}
