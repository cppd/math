/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "directory.h"

#include "name.h"

#include <src/com/file/path.h>

#include <filesystem>
#include <string_view>

namespace ns::settings
{
namespace
{
constexpr std::string_view DIRECTORY_NAME = "test";

void create_dir(const std::filesystem::path& directory)
{
        std::filesystem::create_directory(directory);
        std::filesystem::permissions(directory, std::filesystem::perms::owner_all);
}
}

std::filesystem::path test_directory()
{
        std::filesystem::path directory =
                std::filesystem::temp_directory_path() / path_from_utf8(settings::APPLICATION_NAME);
        create_dir(directory);

        directory = directory / path_from_utf8(DIRECTORY_NAME);
        create_dir(directory);

        return directory;
}
}
