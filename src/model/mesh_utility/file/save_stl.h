/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "../../mesh.h"

#include <filesystem>
#include <string_view>

namespace ns::mesh::file
{
template <std::size_t N>
std::filesystem::path save_to_stl_file(
        const Mesh<N>& mesh,
        const std::filesystem::path& file_name,
        const std::string_view& comment,
        bool ascii_format);
}
