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

#include <src/model/mesh.h>
#include <src/progress/progress.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace ns::model::mesh
{
struct FileFormat final
{
        std::string format_name;
        std::vector<std::string> file_name_extensions;
};

std::vector<FileFormat> save_formats(unsigned dimension);
std::vector<FileFormat> load_formats(const std::set<unsigned>& dimensions);

template <std::size_t N, typename Path>
std::unique_ptr<Mesh<N>> load(const Path& file_name, progress::Ratio* progress);

template <std::size_t N, typename Path>
std::filesystem::path save_to_obj(const Mesh<N>& mesh, const Path& file_name, std::string_view comment);

template <std::size_t N, typename Path>
std::filesystem::path save_to_stl(
        const Mesh<N>& mesh,
        const Path& file_name,
        std::string_view comment,
        bool ascii_format);
}
