/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <cstddef>
#include <string>

namespace ns::text::unicode
{
template <typename T>
std::string utf32_to_number_string(T code_point);

std::string utf8_to_number_string(const std::string& s);

template <typename T>
std::string utf32_to_utf8(T code_point);

char32_t utf8_to_utf32(const std::string& s, std::size_t* i);
char32_t utf8_to_utf32(const std::string& s);

//

inline constexpr char32_t SPACE = 0x20;
inline constexpr char32_t REPLACEMENT_CHARACTER = 0xFFFD;
}
