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

#include <array>
#include <filesystem>
#include <set>
#include <string_view>
#include <vector>

namespace ns::model::mesh::file::obj
{
struct Split final
{
        std::string_view first;
        const char* second_b;
        const char* second_e;
};

[[nodiscard]] Split split_string(std::array<const char*, 2> str);

[[nodiscard]] std::string_view read_name(std::string_view object_name, const char* first, const char* last);

void read_library_names(
        const char* begin,
        const char* end,
        std::vector<std::filesystem::path>* library_names,
        std::set<std::filesystem::path>* unique_library_names);
}
