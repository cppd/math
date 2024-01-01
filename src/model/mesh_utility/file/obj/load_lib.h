/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "../../../mesh.h"

#include <src/progress/progress.h>

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>

namespace ns::model::mesh::file::obj
{
template <std::size_t N>
void read_lib(
        const std::filesystem::path& dir_name,
        const std::filesystem::path& file_name,
        progress::Ratio* progress,
        std::map<std::string, int>* material_index,
        std::map<std::filesystem::path, int>* image_index,
        Mesh<N>* mesh);
}
