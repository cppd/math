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

#include <array>
#include <cmath>
#include <complex>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

std::string source_with_line_numbers(const std::string s);

std::string to_string(unsigned __int128 t);
std::string to_string(__int128 t);
std::string to_string(__float128 t);

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::string> to_string(std::complex<T> t)
{
        std::ostringstream o;
        o << std::setprecision(std::numeric_limits<T>::max_digits10);

        o << (t.real() >= 0 ? " " : "-") << std::abs(t.real());
        o << (t.imag() >= 0 ? " + " : " - ") << std::abs(t.imag()) << "*I";

        return o.str();
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::string> to_string(T t)
{
        std::ostringstream o;
        o << std::setprecision(std::numeric_limits<T>::max_digits10);
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

template <typename T>
std::enable_if_t<std::is_integral_v<T>, std::string> to_string(T t)
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

inline std::string to_string(const glm::vec2& data)
{
        return to_string(std::array<float, 2>{{data[0], data[1]}});
}

inline std::string to_string(const glm::vec3& data)
{
        return to_string(std::array<float, 3>{{data[0], data[1], data[2]}});
}

inline std::string to_string(const glm::vec4& data)
{
        return to_string(std::array<float, 4>{{data[0], data[1], data[2], data[3]}});
}
