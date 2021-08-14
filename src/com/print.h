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

#include "type/detect.h"
#include "type/limit.h"
#include "type/trait.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <complex>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace ns
{
std::string to_string(__float128 t);

template <typename T>
std::string to_string(std::complex<T> t) requires std::is_floating_point_v<T>
{
        std::ostringstream o;
        o << std::setprecision(limits<T>::max_digits10);

        if (t.real() >= 0)
        {
                o << " ";
        }
        else
        {
                o << "-";
        }
        o << std::abs(t.real());

        if (t.imag() >= 0)
        {
                o << " + ";
        }
        else
        {
                o << " - ";
        }
        o << std::abs(t.imag()) << "*I";

        return o.str();
}

template <typename T>
std::string to_string(T t) requires std::is_floating_point_v<T>
{
        std::ostringstream o;
        o << std::setprecision(limits<T>::max_digits10);
        o << t;
        return o.str();
}

template <typename T>
std::string to_string(T t, unsigned digits) requires std::is_floating_point_v<T>
{
        std::ostringstream o;
        o << std::setprecision(digits);
        o << t;
        return o.str();
}

template <typename T>
std::string to_string_fixed(T t, unsigned digits) requires std::is_floating_point_v<T>
{
        std::ostringstream o;
        o << std::setprecision(digits);
        o << std::fixed;
        o << t;

        std::string r = o.str();

        while (!r.empty() && r[r.size() - 1] == '0')
        {
                r.resize(r.size() - 1);
        }

        if (!r.empty() && r[r.size() - 1] == '.')
        {
                r.resize(r.size() - 1);
        }

        if (r.empty())
        {
                return o.str();
        }

        return r;
}

template <typename T>
std::string to_string_binary(T v, const std::string_view& prefix = "") requires std::is_unsigned_v<T>
{
        if (v == 0)
        {
                return std::string(prefix) + '0';
        }
        // std::bit_width
        unsigned width = std::numeric_limits<T>::digits - std::countl_zero(v);
        std::string s(prefix);
        s.resize(s.size() + width);
        for (std::ptrdiff_t i = std::ssize(s) - 1; i >= std::ssize(prefix); --i)
        {
                s[i] = (v & 1u) ? '1' : '0';
                v >>= 1u;
        }
        return s;
}

//

namespace print_implementation
{
template <unsigned digit_group_size, typename T>
void f(T v, int i, std::string& r, [[maybe_unused]] char s)
{
        constexpr bool longlong_less_i128 = limits<long long>::max() < limits<__int128>::max()
                                            && limits<long long>::lowest() > limits<__int128>::lowest();

        constexpr bool ulonglong_less_ui128 = limits<unsigned long long>::max() < limits<unsigned __int128>::max();

        do
        {
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

                int remainder = v % 10;
                if constexpr (is_signed<T>)
                {
                        remainder = std::abs(remainder);
                }
                r += static_cast<char>(remainder + '0');

        } while ((v /= 10) != 0);
}

template <unsigned digit_group_size, typename T>
std::string to_string_digit_groups(T v, char s) requires is_native_integral<T>
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
std::string to_string(T v) requires is_native_integral<T>
{
        return print_implementation::to_string_digit_groups<0, T>(v, '\x20');
}

template <typename T>
std::string to_string_digit_groups(T v, char s = '\x20') requires is_native_integral<T>
{
        return print_implementation::to_string_digit_groups<3, T>(v, s);
}

//

template <typename T>
std::string to_string(const T& data) requires has_cbegin_cend<T>
{
        auto i = std::cbegin(data);
        if (i == std::cend(data))
        {
                return {};
        }
        std::string res = to_string(*i);
        while (++i != std::cend(data))
        {
                res += ", ";
                res += to_string(*i);
        }
        return res;
}

//

std::string to_string(char) = delete;
std::string to_string(wchar_t) = delete;
std::string to_string(char8_t) = delete;
std::string to_string(char16_t) = delete;
std::string to_string(char32_t) = delete;
std::string to_string_digit_groups(char v, char) = delete;
std::string to_string_digit_groups(wchar_t v, char) = delete;
std::string to_string_digit_groups(char8_t v, char) = delete;
std::string to_string_digit_groups(char16_t v, char) = delete;
std::string to_string_digit_groups(char32_t v, char) = delete;

template <typename T>
std::enable_if_t<std::is_pointer_v<T>, std::string> to_string(const T&) = delete;

template <typename T>
std::string to_string(std::basic_string<T>) = delete;
template <typename T>
std::string to_string(std::basic_string_view<T>) = delete;
}
