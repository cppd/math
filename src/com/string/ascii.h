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

#include <algorithm>

namespace ns::ascii
{
constexpr char to_upper(char c)
{
        return c - static_cast<char>(32);
}
constexpr char to_lower(char c)
{
        return c + static_cast<char>(32);
}
constexpr bool is_space(char c)
{
        return c == ' ' || (c <= '\x0d' && c >= '\x09');
}
constexpr bool is_not_space(char c)
{
        return !is_space(c);
}
constexpr bool is_blank(char c)
{
        return c == ' ' || c == '\t';
}
constexpr bool is_not_blank(char c)
{
        return !is_blank(c);
}
constexpr bool is_not_new_line(char c)
{
        return c != '\n';
}
constexpr bool is_digit(char c)
{
        return c >= '0' && c <= '9';
}
constexpr bool is_print(char c)
{
        return c >= '\x20' && c <= '\x7e';
}
constexpr int char_to_int(char c)
{
        return c - '0';
}

template <typename T>
std::enable_if_t<std::is_same_v<std::remove_cvref_t<typename T::value_type>, char>, bool> is_ascii(const T& s)
{
        return std::all_of(
                s.cbegin(), s.cend(),
                [](char c)
                {
                        return static_cast<char8_t>(c) <= 0x7F;
                });
}
}
