/*
Copyright (C) 2017 Topological Manifold

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

constexpr double PI = 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117068;
constexpr const char PI_STR[] =
        "3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117068";
constexpr double TWO_PI = 6.283185307179586476925286766559005768394338798750211641949889184615632812572417997256069650684234136;

constexpr int get_group_count(int size, int group_size) noexcept
{
        return (size + group_size - 1) / group_size;
}

template <typename T>
constexpr T square(const T& v)
{
        return v * v;
}

template <typename T>
T fabs(T a)
{
        static_assert(std::is_floating_point<T>::value);
        return std::fabs(a);
}
template <typename T>
T fma(T a, T b, T c)
{
        static_assert(std::is_floating_point<T>::value);
        return std::fma(a, b, c);
}
template <typename T>
T sqrt(T a)
{
        static_assert(std::is_floating_point<T>::value);
        return std::sqrt(a);
}

#if !defined(__clang__)
inline __float128 fabs(__float128 a)
{
        return fabsq(a);
}
inline __float128 fma(__float128 a, __float128 b, __float128 c)
{
        return fmaq(a, b, c);
}
inline __float128 sqrt(__float128 a)
{
        return sqrtq(a);
}
#endif

template <typename T>
constexpr bool is_finite(T v)
{
        static_assert(std::is_floating_point<T>::value);
        // std::isfinite не работает с -Ofast (-ffast-math),
        // поэтому сравнение с std::numeric_limits<T> lowest и max
        return v >= std::numeric_limits<T>::lowest() && v <= std::numeric_limits<T>::max();
}

constexpr bool is_finite(__float128 v)
{
        // вместо finiteq и по аналогии с другими типами
        return v >= -any_max<__float128> && v <= any_max<__float128>;
}
