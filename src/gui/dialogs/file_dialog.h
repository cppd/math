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

#include <optional>
#include <string>
#include <vector>

namespace ns::gui::dialogs
{
struct FileFilter final
{
        std::string name;
        std::vector<std::string> file_extensions;
};

[[nodiscard]] std::optional<std::string> save_file(
        const std::string& caption,
        const std::vector<FileFilter>& filters,
        bool read_only);

[[nodiscard]] std::optional<std::string> save_file(
        const std::string& caption,
        const std::string& file_name,
        const std::vector<FileFilter>& filters,
        bool read_only);

[[nodiscard]] std::optional<std::string> open_file(
        const std::string& caption,
        const std::vector<FileFilter>& filters,
        bool read_only);

[[nodiscard]] std::optional<std::string> select_directory(const std::string& caption, bool read_only);
}
