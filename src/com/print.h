/*
Copyright (C) 2017-2022 Topological Manifold

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
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace ns
{
namespace print_implementation
{
template <unsigned DIGIT_GROUP_SIZE, typename T>
void f(T v, int i, std::string& r, [[maybe_unused]] const char s)
{
        constexpr bool LONGLONG_LESS_I128 = Limits<long long>::max() < Limits<__int128>::max()
                                            && Limits<long long>::lowest() > Limits<__int128>::lowest();

        constexpr bool ULONGLONG_LESS_UI128 = Limits<unsigned long long>::max() < Limits<unsigned __int128>::max();

        do
        {
                if constexpr (std::is_same_v<__int128, std::remove_cv_t<T>> && LONGLONG_LESS_I128)
                {
                        if (Limits<long long>::lowest() <= v && v <= Limits<long long>::max())
                        {
                                f<DIGIT_GROUP_SIZE, long long>(v, i, r, s);
                                break;
                        }
                }
                if constexpr (std::is_same_v<unsigned __int128, std::remove_cv_t<T>> && ULONGLONG_LESS_UI128)
                {
                        if (v <= Limits<unsigned long long>::max())
                        {
                                f<DIGIT_GROUP_SIZE, unsigned long long>(v, i, r, s);
                                break;
                        }
                }

                if constexpr (DIGIT_GROUP_SIZE > 0)
                {
                        if ((++i % DIGIT_GROUP_SIZE) == 0 && i != 0)
                        {
                                r += s;
                        }
                }

                int remainder = v % 10;
                if constexpr (Signed<T>)
                {
                        remainder = std::abs(remainder);
                }
                r += static_cast<char>(remainder + '0');

        } while ((v /= 10) != 0);
}

template <unsigned DIGIT_GROUP_SIZE, typename T>
std::string to_string_digit_groups(const T& v, const char s) requires Integral<T>
{
        static_assert(!std::is_class_v<T>);
        static_assert(Signed<T> != Unsigned<T>);

        std::string r;
        r.reserve(Limits<T>::digits10() * 1.5);

        f<DIGIT_GROUP_SIZE, T>(v, -1, r, s);

        if (Signed<T> && v < 0)
        {
                r += '-';
        }

        std::reverse(r.begin(), r.end());

        return r;
}
}

std::string to_string(__float128 t);

template <typename T>
std::string to_string(const std::complex<T> t) requires std::is_floating_point_v<T>
{
        std::ostringstream o;
        o << std::setprecision(Limits<T>::max_digits10());

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
std::string to_string(const T t) requires std::is_floating_point_v<T>
{
        std::ostringstream o;
        o << std::setprecision(Limits<T>::max_digits10());
        o << t;
        return o.str();
}

template <typename T>
std::string to_string(const T t, const unsigned digits) requires std::is_floating_point_v<T>
{
        std::ostringstream o;
        o << std::setprecision(digits);
        o << t;
        return o.str();
}

template <typename T>
std::string to_string_fixed(const T t, const unsigned digits) requires std::is_floating_point_v<T>
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
        unsigned width = std::bit_width(v);
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

template <typename T>
std::string to_string(const T v) requires Integral<T>
{
        return print_implementation::to_string_digit_groups<0, T>(v, '\x20');
}

template <typename T>
std::string to_string_digit_groups(const T v, const char s = '\x20') requires Integral<T>
{
        return print_implementation::to_string_digit_groups<3, T>(v, s);
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
std::string to_string(const T&) requires(std::is_pointer_v<T>) = delete;

template <typename T>
std::string to_string(std::basic_string<T>) = delete;
template <typename T>
std::string to_string(std::basic_string_view<T>) = delete;

//

template <typename T>
std::string to_string(const T& data) requires requires
{
        std::begin(data);
        std::end(data);
}
{
        auto i = std::begin(data);
        if (i == std::end(data))
        {
                return {};
        }
        std::string res = to_string(*i);
        while (++i != std::end(data))
        {
                res += ", ";
                res += to_string(*i);
        }
        return res;
}
}
