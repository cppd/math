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

#pragma once

#include "limit.h"

#include <array>
#include <bit>
#include <type_traits>

namespace ns
{
namespace type_name_implementation
{
template <typename T>
concept StdFloatingPoint = (std::is_same_v<std::remove_cv_t<T>, float>) || (std::is_same_v<std::remove_cv_t<T>, double>)
                           || (std::is_same_v<std::remove_cv_t<T>, long double>);
template <typename T>
concept BitFloatingPoint = (StdFloatingPoint<T> && std::numeric_limits<T>::is_iec559)
                           || (std::is_same_v<std::remove_cv_t<T>, __float128>);

template <BitFloatingPoint T>
constexpr unsigned floating_point_bit_count()
{
        constexpr unsigned max_exponent = limits<T>::max_exponent;
        static_assert(1 == std::popcount(max_exponent));

        constexpr unsigned size = limits<T>::digits + 1 + std::countr_zero(max_exponent);
        if constexpr (!std::is_same_v<std::remove_cv_t<T>, long double>)
        {
                return size;
        }
        else
        {
                return size == 79 ? 80 : size;
        }
}

static_assert(32 == floating_point_bit_count<float>());
static_assert(64 == floating_point_bit_count<double>());
static_assert(128 == floating_point_bit_count<__float128>());
}

//

template <typename T>
constexpr const char* type_name() requires(std::is_same_v<std::remove_cv_t<T>, float>)
{
        return "float";
}

template <typename T>
constexpr const char* type_name() requires(std::is_same_v<std::remove_cv_t<T>, double>)
{
        return "double";
}

template <typename T>
constexpr const char* type_name() requires(std::is_same_v<std::remove_cv_t<T>, long double>)
{
        return "long double";
}

template <typename T>
constexpr const char* type_name() requires(std::is_same_v<std::remove_cv_t<T>, __float128>)
{
        return "__float128";
}

//

template <type_name_implementation::BitFloatingPoint T>
const char* type_bit_name()
{
        static constexpr std::size_t size = type_name_implementation::floating_point_bit_count<T>();
        static_assert(size >= 10 && size <= 999);

        if constexpr (size < 100)
        {
                static constexpr std::array str = std::to_array<char>({'f', 'p', '0' + size / 10, '0' + size % 10, 0});
                return str.data();
        }
        else
        {
                static constexpr std::array str =
                        std::to_array<char>({'f', 'p', '0' + size / 100, '0' + (size % 100) / 10, '0' + size % 10, 0});
                return str.data();
        }
}

//

template <typename T>
constexpr const char* floating_point_suffix() requires(std::is_same_v<std::remove_cv_t<T>, float>)
{
        return "f";
}

template <typename T>
constexpr const char* floating_point_suffix() requires(std::is_same_v<std::remove_cv_t<T>, double>)
{
        return "";
}

template <typename T>
constexpr const char* floating_point_suffix() requires(std::is_same_v<std::remove_cv_t<T>, long double>)
{
        return "l";
}
}
