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
#ifndef PRINT_H
#define PRINT_H

#include <array>
#include <cmath>
#include <complex>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#if !defined(__clang__)
#include <quadmath.h>
#endif

std::string source_with_line_numbers(const std::string s);

#if !defined(__clang__)
inline std::string to_string(__float128 t)
{
        constexpr const char* QUAD_MATH_FORMAT = "%.36Qe"; //"%+-#*.36Qe"

        std::array<char, 1000> buf;
        quadmath_snprintf(buf.data(), buf.size(), QUAD_MATH_FORMAT, t);
        return buf.data();
}
#endif

std::string to_string(unsigned __int128 t);
std::string to_string(__int128 t);

template <typename T>
std::enable_if_t<std::is_floating_point<T>::value, std::string> to_string(std::complex<T> t)
{
        std::ostringstream o;
        o << std::setprecision(std::numeric_limits<T>::max_digits10);

        o << (t.real() >= 0 ? " " : "-") << std::fabs(t.real());
        o << (t.imag() >= 0 ? " + " : " - ") << std::fabs(t.imag()) << "*I";

        return o.str();
}

template <typename T>
std::enable_if_t<std::is_floating_point<T>::value, std::string> to_string(T t)
{
        std::ostringstream o;
        o << std::setprecision(std::numeric_limits<T>::max_digits10);
        o << t;
        return o.str();
}

template <typename T>
std::enable_if_t<std::is_floating_point<T>::value, std::string> to_string(T t, unsigned digits)
{
        std::ostringstream o;
        o << std::setprecision(digits);
        o << t;
        return o.str();
}

template <typename T>
std::enable_if_t<std::is_integral<T>::value, std::string> to_string(T t)
{
        return std::to_string(t);
}

template <typename T, size_t N>
std::string to_string(const std::array<T, N>& data)
{
        std::string o;

        for (size_t i = 0; i < data.size(); ++i)
        {
                o += to_string(data[i]);

                if (i != data.size() - 1)
                {
                        o += ", ";
                }
        }
        return o;
}

template <typename T>
std::string to_string(const std::vector<T>& data)
{
        std::string o;

        for (size_t i = 0; i < data.size(); ++i)
        {
                o += to_string(data[i]);

                if (i != data.size() - 1)
                {
                        o += ", ";
                }
        }
        return o;
}

#endif
