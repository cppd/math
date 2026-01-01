/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <string>
#include <string_view>

namespace ns
{
[[nodiscard]] std::string trim(std::string_view s);
[[nodiscard]] std::string to_upper(std::string_view s);
[[nodiscard]] std::string to_lower(std::string_view s);
[[nodiscard]] std::string to_upper_first_letters(std::string_view s);
[[nodiscard]] std::string add_indent(std::string_view s, unsigned indent_size);
[[nodiscard]] std::string printable_characters(std::string_view s);
[[nodiscard]] std::string replace_space(std::string_view s, char value);
}
