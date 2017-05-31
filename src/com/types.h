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
#ifndef TYPES_H
#define TYPES_H

#include "error.h"

#include <gmpxx.h>
#include <string>
#include <type_traits>

// Не работает с GCC и -Ofast, но работает с clang и -Ofast
// template <typename T>
// constexpr T binary_epsilon()
//{
//        T prev_e = 1, e = 1;
//        do
//        {
//                prev_e = e;
//                e = e / 2;
//        } while (1 != 1 + e);
//        return prev_e;
//}
template <typename T>
constexpr T max_binary_fraction()
{
        // = 2 - epsilon
        T prev_r = 1, r = 1, a = 1;
        do
        {
                prev_r = r;
                a /= 2;
                r += a;
        } while (r != 2);
        return prev_r;
}
template <typename T>
constexpr T binary_exponent(int e)
{
        if (e == 0)
        {
                return 1;
        }
        T r = 1;
        if (e > 0)
        {
                for (int i = 1; i <= e; ++i)
                {
                        r *= 2;
                }
        }
        if (e < 0)
        {
                e = -e;
                for (int i = 1; i <= e; ++i)
                {
                        r /= 2;
                }
        }
        return r;
}

// template <typename T>
// constexpr T any_epsilon = binary_epsilon<T>();
template <typename T>
constexpr T any_epsilon = std::numeric_limits<T>::epsilon();
template <>
constexpr __float128 any_epsilon<__float128> = 1e-34; // strtoflt128("1.92592994438723585305597794258492732e-34", nullptr)

// Проверка правильности работы функций
static_assert((1 + any_epsilon<float> != 1) && (1 + any_epsilon<float> / 2 == 1));
static_assert((1 + any_epsilon<double> != 1) && (1 + any_epsilon<double> / 2 == 1));
static_assert((1 + any_epsilon<__float128> != 1) && (1 + any_epsilon<__float128> / 2 == 1));
static_assert(2 - any_epsilon<float> == max_binary_fraction<float>());
static_assert(2 - any_epsilon<double> == max_binary_fraction<double>());
static_assert(2 - any_epsilon<__float128> == max_binary_fraction<__float128>());
static_assert(std::numeric_limits<float>::max() == max_binary_fraction<float>() * binary_exponent<float>(127));
static_assert(std::numeric_limits<double>::max() == max_binary_fraction<double>() * binary_exponent<double>(1023));

// strtoflt128("1.18973149535723176508575932662800702e4932", nullptr)
constexpr __float128 MAX_FLOAT_128 = max_binary_fraction<__float128>() * binary_exponent<__float128>(16383);

// constexpr unsigned __int128 MAX_UNSIGNED_INT_128 = -1;
constexpr unsigned __int128 MAX_UNSIGNED_INT_128 =
        (static_cast<unsigned __int128>(0xffff'ffff'ffff'ffff) << 64) | static_cast<unsigned __int128>(0xffff'ffff'ffff'ffff);
static_assert(MAX_UNSIGNED_INT_128 + 1 == 0);

constexpr signed __int128 MAX_SIGNED_INT_128 = MAX_UNSIGNED_INT_128 >> 1;
static_assert(MAX_SIGNED_INT_128 > 0 && (static_cast<unsigned __int128>(MAX_SIGNED_INT_128) == MAX_UNSIGNED_INT_128 >> 1));

template <typename T>
constexpr T any_max = std::numeric_limits<T>::max();
template <>
constexpr unsigned __int128 any_max<unsigned __int128> = MAX_UNSIGNED_INT_128;
template <>
constexpr signed __int128 any_max<signed __int128> = MAX_SIGNED_INT_128;
template <>
constexpr __float128 any_max<__float128> = MAX_FLOAT_128;

template <typename T>
constexpr int any_digits = std::numeric_limits<T>::digits;
template <>
constexpr int any_digits<unsigned __int128> = 128;
template <>
constexpr int any_digits<signed __int128> = 127;
template <>
constexpr int any_digits<__float128> = 113;

template <typename T>
constexpr bool native_integral = std::disjunction<std::is_same<std::remove_const_t<T>, __int128>, std::is_integral<T>>::value;

template <typename T>
constexpr bool any_integral = std::disjunction<std::is_same<std::remove_const_t<T>, mpz_class>,
                                               std::is_same<std::remove_const_t<T>, __int128>, std::is_integral<T>>::value;

template <typename T>
constexpr bool native_floating_point =
        std::disjunction<std::is_same<std::remove_const_t<T>, __float128>, std::is_floating_point<T>>::value;

template <typename T>
constexpr bool any_floating_point =
        std::disjunction<std::is_same<std::remove_const_t<T>, mpf_class>, std::is_same<std::remove_const_t<T>, __float128>,
                         std::is_floating_point<T>>::value;

template <typename T>
constexpr bool any_signed =
        std::disjunction<std::is_same<std::remove_const_t<T>, mpz_class>, std::is_same<std::remove_const_t<T>, mpf_class>,
                         std::is_same<std::remove_const_t<T>, __int128>, std::is_same<std::remove_const_t<T>, __float128>,
                         std::is_signed<T>>::value;

// clang-format off
template<int BITS>
using LeastSignedInteger =
        std::conditional_t<BITS <=   8, int_least8_t,
        std::conditional_t<BITS <=  16, int_least16_t,
        std::conditional_t<BITS <=  32, int_least32_t,
        std::conditional_t<BITS <=  64, int_least64_t,
        std::conditional_t<BITS <= 128, signed __int128,
        mpz_class>>>>>;
template<int BITS>
using LeastUnsignedInteger =
        std::conditional_t<BITS <=   8, uint_least8_t,
        std::conditional_t<BITS <=  16, uint_least16_t,
        std::conditional_t<BITS <=  32, uint_least32_t,
        std::conditional_t<BITS <=  64, uint_least64_t,
        std::conditional_t<BITS <= 128, unsigned __int128,
        mpz_class>>>>>;
// clang-format on

template <typename T>
std::string type_str()
{
        if (std::is_same<std::remove_const_t<T>, mpz_class>::value)
        {
                return "mpz_class";
        }
        if (std::is_same<std::remove_const_t<T>, mpf_class>::value)
        {
                return "mpf_class";
        }
        if (std::is_same<std::remove_const_t<T>, mpq_class>::value)
        {
                return "mpq_class";
        }
        if (any_integral<T>)
        {
                return std::to_string(any_digits<T>) + " bits";
        }
        if (any_floating_point<T>)
        {
                return "fp " + std::to_string(any_digits<T>) + " bits";
        }
        error("data type string: type not supported");
}

#endif
