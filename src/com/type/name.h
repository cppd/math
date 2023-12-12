/*
Copyright (C) 2017-2023 Topological Manifold

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
#include <cstddef>
#include <type_traits>

namespace ns
{
namespace type_name_implementation
{
template <typename T>
concept StdFloatingPoint =
        (std::is_same_v<std::remove_cv_t<T>, float>) || (std::is_same_v<std::remove_cv_t<T>, double>)
        || (std::is_same_v<std::remove_cv_t<T>, long double>);
template <typename T>
concept BitFloatingPoint =
        (StdFloatingPoint<T> && Limits<T>::is_iec559()) || (std::is_same_v<std::remove_cv_t<T>, __float128>);

template <BitFloatingPoint T>
constexpr unsigned floating_point_bit_count()
{
        constexpr unsigned MAX_EXPONENT = Limits<T>::max_exponent();
        static_assert(1 == std::popcount(MAX_EXPONENT));

        constexpr unsigned SIZE = Limits<T>::digits() + 1 + std::countr_zero(MAX_EXPONENT);
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
        requires (std::is_same_v<std::remove_cv_t<T>, float>)
[[nodiscard]] constexpr const char* type_name()
{
        return "float";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, double>)
[[nodiscard]] constexpr const char* type_name()
{
        return "double";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, long double>)
[[nodiscard]] constexpr const char* type_name()
{
        return "long double";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, __float128>)
[[nodiscard]] constexpr const char* type_name()
{
        return "__float128";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, bool>)
[[nodiscard]] constexpr const char* type_name()
{
        return "bool";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, char>)
[[nodiscard]] constexpr const char* type_name()
{
        return "char";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, signed char>)
[[nodiscard]] constexpr const char* type_name()
{
        return "signed char";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, unsigned char>)
[[nodiscard]] constexpr const char* type_name()
{
        return "unsigned char";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, char8_t>)
[[nodiscard]] constexpr const char* type_name()
{
        return "char8_t";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, char16_t>)
[[nodiscard]] constexpr const char* type_name()
{
        return "char16_t";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, char32_t>)
[[nodiscard]] constexpr const char* type_name()
{
        return "char32_t";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, short>)
[[nodiscard]] constexpr const char* type_name()
{
        return "short";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, unsigned short>)
[[nodiscard]] constexpr const char* type_name()
{
        return "unsigned short";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, int>)
[[nodiscard]] constexpr const char* type_name()
{
        return "int";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, unsigned int>)
[[nodiscard]] constexpr const char* type_name()
{
        return "unsigned int";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, long>)
[[nodiscard]] constexpr const char* type_name()
{
        return "long";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, unsigned long>)
[[nodiscard]] constexpr const char* type_name()
{
        return "unsigned long";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, long long>)
[[nodiscard]] constexpr const char* type_name()
{
        return "long long";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, unsigned long long>)
[[nodiscard]] constexpr const char* type_name()
{
        return "unsigned long long";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, __int128>)
[[nodiscard]] constexpr const char* type_name()
{
        return "__int128";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, unsigned __int128>)
[[nodiscard]] constexpr const char* type_name()
{
        return "unsigned __int128";
}

//

template <type_name_implementation::BitFloatingPoint T>
[[nodiscard]] const char* type_bit_name()
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

template <typename T>
        requires (
                (std::is_integral_v<T> && std::is_signed_v<T>)
                || (std::is_same_v<std::remove_cv_t<T>, signed __int128>))
[[nodiscard]] const char* type_bit_name()
{
        static constexpr std::size_t SIZE = 1 + Limits<T>::digits();
        static_assert(SIZE >= 1 && SIZE <= 999);

        if constexpr (SIZE < 10)
        {
                static constexpr std::array STR = std::to_array<char>({'i', 'n', 't', '0' + SIZE, 0});
                return STR.data();
        }
        else if constexpr (SIZE < 100)
        {
                static constexpr std::array STR =
                        std::to_array<char>({'i', 'n', 't', '0' + SIZE / 10, '0' + SIZE % 10, 0});
                return STR.data();
        }
        else
        {
                static constexpr std::array STR = std::to_array<char>(
                        {'i', 'n', 't', '0' + SIZE / 100, '0' + (SIZE % 100) / 10, '0' + SIZE % 10, 0});
                return STR.data();
        }
}

template <typename T>
        requires (
                (std::is_integral_v<T> && std::is_unsigned_v<T>)
                || (std::is_same_v<std::remove_cv_t<T>, unsigned __int128>))
[[nodiscard]] const char* type_bit_name()
{
        static constexpr std::size_t SIZE = Limits<T>::digits();
        static_assert(SIZE >= 1 && SIZE <= 999);

        if constexpr (SIZE < 10)
        {
                static constexpr std::array STR = std::to_array<char>({'u', 'i', 'n', 't', '0' + SIZE, 0});
                return STR.data();
        }
        else if constexpr (SIZE < 100)
        {
                static constexpr std::array STR =
                        std::to_array<char>({'u', 'i', 'n', 't', '0' + SIZE / 10, '0' + SIZE % 10, 0});
                return STR.data();
        }
        else
        {
                static constexpr std::array STR = std::to_array<char>(
                        {'u', 'i', 'n', 't', '0' + SIZE / 100, '0' + (SIZE % 100) / 10, '0' + SIZE % 10, 0});
                return STR.data();
        }
}

//

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, float>)
[[nodiscard]] constexpr const char* floating_point_suffix()
{
        return "f";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, double>)
[[nodiscard]] constexpr const char* floating_point_suffix()
{
        return "";
}

template <typename T>
        requires (std::is_same_v<std::remove_cv_t<T>, long double>)
[[nodiscard]] constexpr const char* floating_point_suffix()
{
        return "l";
}
}
