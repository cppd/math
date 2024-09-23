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

#pragma once

#include "type/concept.h"
#include "type/limit.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <complex>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace ns
{
namespace print_implementation
{
template <typename T>
char digit(const T value)
{
        const int remainder = [&]
        {
                const int res = value % 10;
                if constexpr (Signed<T>)
                {
                        return std::abs(res);
                }
                return res;
        }();
        return remainder + '0';
}

template <typename T>
inline constexpr bool USE_SIGNED_LONG_LONG =
        std::is_same_v<signed __int128, std::remove_cv_t<T>>
        && (Limits<long long>::max() < Limits<signed __int128>::max())
        && (Limits<long long>::lowest() > Limits<signed __int128>::lowest());

template <typename T>
inline constexpr bool USE_UNSIGNED_LONG_LONG =
        std::is_same_v<unsigned __int128, std::remove_cv_t<T>>
        && (Limits<unsigned long long>::max() < Limits<unsigned __int128>::max());

template <typename T>
using LongLongType = std::conditional_t<
        USE_SIGNED_LONG_LONG<T>,
        long long,
        std::conditional_t<USE_UNSIGNED_LONG_LONG<T>, unsigned long long, void>>;

template <unsigned DIGIT_GROUP_SIZE, typename T>
void make_string(T value, int index, [[maybe_unused]] const char separator, std::string& str)
{
        do
        {
                if constexpr (USE_SIGNED_LONG_LONG<T> || USE_UNSIGNED_LONG_LONG<T>)
                {
                        const LongLongType<T> v = value;
                        if (v == value)
                        {
                                make_string<DIGIT_GROUP_SIZE>(v, index, separator, str);
                                return;
                        }
                }

                if constexpr (DIGIT_GROUP_SIZE > 0)
                {
                        ++index;
                        if ((index % DIGIT_GROUP_SIZE) == 0 && index != 0)
                        {
                                str += separator;
                        }
                }

                str += digit(value);

        } while ((value /= 10) != 0);
}

template <unsigned DIGIT_GROUP_SIZE, typename T>
        requires Integral<T>
[[nodiscard]] std::string to_string_digit_groups(const T value, const char separator)
{
        static_assert(!std::is_class_v<T>);
        static_assert(Signed<T> != Unsigned<T>);

        std::string res;
        res.reserve(Limits<T>::digits10() * 2);

        make_string<DIGIT_GROUP_SIZE, T>(value, -1, separator, res);

        if (Signed<T> && value < 0)
        {
                res += '-';
        }

        std::ranges::reverse(res);

        return res;
}
}

[[nodiscard]] std::string to_string(__float128 value);

template <typename T>
        requires std::is_floating_point_v<T>
[[nodiscard]] std::string to_string(const std::complex<T> value)
{
        std::ostringstream oss;
        oss << std::setprecision(Limits<T>::max_digits10());

        if (value.real() >= 0)
        {
                oss << " ";
        }
        else
        {
                oss << "-";
        }
        oss << std::abs(value.real());

        if (value.imag() >= 0)
        {
                oss << " + ";
        }
        else
        {
                oss << " - ";
        }
        oss << std::abs(value.imag()) << "*I";

        return oss.str();
}

template <typename T>
        requires std::is_floating_point_v<T>
[[nodiscard]] std::string to_string(const T value)
{
        std::ostringstream oss;
        oss << std::setprecision(Limits<T>::max_digits10());
        oss << value;
        return oss.str();
}

template <typename T>
        requires std::is_floating_point_v<T>
[[nodiscard]] std::string to_string(const T value, const unsigned digits)
{
        std::ostringstream oss;
        oss << std::setprecision(digits);
        oss << value;
        return oss.str();
}

template <typename T>
        requires std::is_floating_point_v<T>
[[nodiscard]] std::string to_string_fixed(const T value, const unsigned digits)
{
        std::ostringstream oss;
        oss << std::setprecision(digits);
        oss << std::fixed;
        oss << value;

        std::string res = oss.str();

        while (!res.empty() && res[res.size() - 1] == '0')
        {
                res.resize(res.size() - 1);
        }

        if (!res.empty() && res[res.size() - 1] == '.')
        {
                res.resize(res.size() - 1);
        }

        if (res.empty())
        {
                return oss.str();
        }

        return res;
}

template <typename T>
        requires std::is_unsigned_v<T>
[[nodiscard]] std::string to_string_binary(T value, const std::string_view prefix = {})
{
        if (value == 0)
        {
                if (prefix.empty())
                {
                        return "0";
                }
                return std::string(prefix) + '0';
        }

        std::string res(prefix);
        res.resize(res.size() + std::bit_width(value));
        for (std::ptrdiff_t i = std::ssize(res) - 1; i >= std::ssize(prefix); --i)
        {
                res[i] = (value & 1u) ? '1' : '0';
                value >>= 1u;
        }
        return res;
}

//

template <typename T>
        requires Integral<T>
[[nodiscard]] std::string to_string(const T value)
{
        return print_implementation::to_string_digit_groups<0, T>(value, '\x20');
}

template <typename T>
        requires Integral<T>
[[nodiscard]] std::string to_string_digit_groups(const T value, const char separator = '\x20')
{
        return print_implementation::to_string_digit_groups<3, T>(value, separator);
}

//

void to_string(char) = delete;
void to_string(wchar_t) = delete;
void to_string(char8_t) = delete;
void to_string(char16_t) = delete;
void to_string(char32_t) = delete;
void to_string_digit_groups(char v, char) = delete;
void to_string_digit_groups(wchar_t v, char) = delete;
void to_string_digit_groups(char8_t v, char) = delete;
void to_string_digit_groups(char16_t v, char) = delete;
void to_string_digit_groups(char32_t v, char) = delete;

template <typename T>
        requires std::is_pointer_v<T>
void to_string(const T&) = delete;

template <typename T>
void to_string(std::basic_string<T>) = delete;
template <typename T>
void to_string(std::basic_string_view<T>) = delete;

//

template <typename T>
[[nodiscard]] std::string to_string(const T& data)
        requires requires {
                         std::begin(data);
                         std::end(data);
                 }
{
        auto i = std::begin(data);
        if (i == std::end(data))
        {
                return {};
        }

        constexpr bool IS_CLASS = std::is_class_v<std::remove_cvref<decltype(*i)>>;

        std::string res = to_string(*i);
        while (++i != std::end(data))
        {
                if constexpr (IS_CLASS)
                {
                        res += '\n';
                }
                else
                {
                        res += ", ";
                }
                res += to_string(*i);
        }
        return res;
}
}
