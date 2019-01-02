/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/type/detect.h"
#include "com/type/limit.h"
#include "com/type/trait.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <iomanip>
#include <sstream>
#include <string>

//

std::string source_with_line_numbers(const std::string s);

//

std::string to_string(__float128 t);

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::string> to_string(std::complex<T> t)
{
        std::ostringstream o;
        o << std::setprecision(limits<T>::max_digits10);

        o << (t.real() >= 0 ? " " : "-") << std::abs(t.real());
        o << (t.imag() >= 0 ? " + " : " - ") << std::abs(t.imag()) << "*I";

        return o.str();
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::string> to_string(T t)
{
        std::ostringstream o;
        o << std::setprecision(limits<T>::max_digits10);
        o << t;
        return o.str();
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::string> to_string(T t, unsigned digits)
{
        std::ostringstream o;
        o << std::setprecision(digits);
        o << t;
        return o.str();
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::string> to_string_fixed(T t, unsigned digits)
{
        std::ostringstream o;
        o << std::setprecision(digits);
        o << std::fixed;
        o << t;

        std::string r = o.str();

        while (r.size() > 0 && r[r.size() - 1] == '0')
        {
                r.resize(r.size() - 1);
        }

        if (r.size() > 0 && r[r.size() - 1] == '.')
        {
                r.resize(r.size() - 1);
        }

        if (r.size() == 0)
        {
                return o.str();
        }

        return r;
}

//

namespace integral_to_string_implementation
{
template <unsigned digit_group_size, typename T>
void f(T v, int i, std::string& r, [[maybe_unused]] char s)
{
        constexpr bool longlong_less_i128 =
                limits<long long>::max() < limits<__int128>::max() && limits<long long>::lowest() > limits<__int128>::lowest();

        constexpr bool ulonglong_less_ui128 = limits<unsigned long long>::max() < limits<unsigned __int128>::max();

        do
        {
                // Быстрее работает, если переключаться с __int128 на long long
                if constexpr (std::is_same_v<__int128, std::remove_cv_t<T>> && longlong_less_i128)
                {
                        if (limits<long long>::lowest() <= v && v <= limits<long long>::max())
                        {
                                f<digit_group_size, long long>(v, i, r, s);
                                break;
                        }
                }
                if constexpr (std::is_same_v<unsigned __int128, std::remove_cv_t<T>> && ulonglong_less_ui128)
                {
                        if (v <= limits<unsigned long long>::max())
                        {
                                f<digit_group_size, unsigned long long>(v, i, r, s);
                                break;
                        }
                }

                if constexpr (digit_group_size > 0)
                {
                        if ((++i % digit_group_size) == 0 && i != 0)
                        {
                                r += s;
                        }
                }

                if constexpr (is_signed<T>)
                {
                        r += std::abs(static_cast<signed char>(v % 10)) + '0';
                }
                else
                {
                        r += static_cast<unsigned char>(v % 10) + '0';
                }

        } while ((v /= 10) != 0);
}

template <unsigned digit_group_size, typename T>
std::enable_if_t<is_native_integral<T>, std::string> to_string_digit_groups(T v, char s)
{
        static_assert(is_signed<T> != is_unsigned<T>);

        std::string r;
        r.reserve(limits<T>::digits10 * 1.5);

        bool negative = is_unsigned<T> ? false : v < 0;

        f<digit_group_size, T>(v, -1, r, s);

        if (negative)
        {
                r += '-';
        }

        std::reverse(r.begin(), r.end());

        return r;
}
}

template <typename T>
std::enable_if_t<is_native_integral<T>, std::string> to_string(T v)
{
        return integral_to_string_implementation::to_string_digit_groups<0, T>(v, '\x20');
}

template <typename T>
std::enable_if_t<is_native_integral<T>, std::string> to_string_digit_groups(T v, char s = '\x20')
{
        return integral_to_string_implementation::to_string_digit_groups<3, T>(v, s);
}

std::string to_string(char) = delete;
std::string to_string(wchar_t) = delete;
std::string to_string(char16_t) = delete;
std::string to_string(char32_t) = delete;
std::string to_string_digit_groups(char v, char s = '\x20') = delete;
std::string to_string_digit_groups(wchar_t v, char s = '\x20') = delete;
std::string to_string_digit_groups(char16_t v, char s = '\x20') = delete;
std::string to_string_digit_groups(char32_t v, char s = '\x20') = delete;

//

template <typename T>
std::enable_if_t<has_begin_end<T>, std::string> to_string(const T& data)
{
        std::string res;

        for (auto i = std::cbegin(data); i != std::cend(data); ++i)
        {
                if (i != std::cbegin(data))
                {
                        res += ", ";
                }

                res += to_string(*i);
        }

        return res;
}

//

template <typename T>
std::string to_string(const T*) = delete;

std::string to_string(const char*) = delete;
std::string to_string(const wchar_t*) = delete;
std::string to_string(const char16_t*) = delete;
std::string to_string(const char32_t*) = delete;
template <typename T>
std::string to_string(std::basic_string<T>) = delete;
template <typename T>
std::string to_string(std::basic_string_view<T>) = delete;
