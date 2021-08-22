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
        constexpr unsigned MAX_EXPONENT = limits<T>::max_exponent;
        static_assert(1 == std::popcount(MAX_EXPONENT));

        constexpr unsigned SIZE = limits<T>::digits + 1 + std::countr_zero(MAX_EXPONENT);
        if constexpr (!std::is_same_v<std::remove_cv_t<T>, long double>)
        {
                return SIZE;
        }
        else
        {
                return SIZE == 79 ? 80 : SIZE;
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
        static constexpr std::size_t SIZE = type_name_implementation::floating_point_bit_count<T>();
        static_assert(SIZE >= 10 && SIZE <= 999);

        if constexpr (SIZE < 100)
        {
                static constexpr std::array STR = std::to_array<char>({'f', 'p', '0' + SIZE / 10, '0' + SIZE % 10, 0});
                return STR.data();
        }
        else
        {
                static constexpr std::array STR =
                        std::to_array<char>({'f', 'p', '0' + SIZE / 100, '0' + (SIZE % 100) / 10, '0' + SIZE % 10, 0});
                return STR.data();
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
