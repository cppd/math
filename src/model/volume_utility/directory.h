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

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace ns::model::volume
{
enum class ContentType
{
        FILES,
        DIRECTORIES
};

struct DirectoryInfo final
{
        ContentType type;
        std::size_t count;
        std::string first;
};

template <typename Path>
std::optional<DirectoryInfo> read_directory_info(const Path& directory);

template <typename Path>
std::vector<std::string> read_directories(const Path& directory);

template <typename Path>
std::vector<std::string> read_files(const Path& directory);
}
