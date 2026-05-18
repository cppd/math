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

#include "directory.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/string/ascii.h>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ns::model::volume
{
namespace
{
struct DirectoryContent final
{
        ContentType type;
        std::vector<std::string> entries;
};

bool directory_contains_files(
        const std::filesystem::path& directory,
        const std::filesystem::directory_entry& entry,
        const std::optional<bool> contains_files)
{
        if (entry.is_directory())
        {
                if (contains_files && *contains_files)
                {
                        error("Mixed content found in directory " + generic_utf8_filename(directory));
                }
                return false;
        }

        if (entry.is_regular_file())
        {
                if (contains_files && !*contains_files)
                {
                        error("Mixed content found in directory " + generic_utf8_filename(directory));
                }
                return true;
        }

        error("Neither directory nor regular file found " + generic_utf8_filename(entry.path()));
}

std::string entry_name(const std::filesystem::directory_entry& entry)
{
        std::string name = generic_utf8_filename(entry.path().filename());
        if (!ascii::is_ascii(name))
        {
                error("Directory entry does not have only ASCII encoding " + generic_utf8_filename(entry.path()));
        }
        return name;
}

template <typename Path>
DirectoryContent read_directory(const Path& directory)
{
        if (!std::filesystem::is_directory(directory))
        {
                error("Directory not found " + generic_utf8_filename(directory));
        }

        std::vector<std::string> entries;
        std::optional<bool> contains_files;

        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory))
        {
                contains_files = directory_contains_files(directory, entry, contains_files);
                entries.push_back(entry_name(entry));
        }

        if (entries.empty())
        {
                return {};
        }

        ASSERT(contains_files.has_value());

        return {
                .type = (*contains_files ? ContentType::FILES : ContentType::DIRECTORIES),
                .entries = std::move(entries),
        };
}
}

template <typename Path>
std::optional<DirectoryInfo> read_directory_info(const Path& directory)
{
        DirectoryContent content = read_directory(directory);

        if (content.entries.empty())
        {
                return {};
        }

        return {
                {.type = content.type,
                 .count = content.entries.size(),
                 .first = std::move(*std::ranges::min_element(content.entries))}
        };
}

template <typename Path>
std::vector<std::string> read_directories(const Path& directory)
{
        DirectoryContent content = read_directory(directory);

        if (content.entries.empty())
        {
                error("Directories not found in " + generic_utf8_filename(directory));
        }

        if (content.type == ContentType::DIRECTORIES)
        {
                return std::move(content.entries);
        }

        error("Directory " + generic_utf8_filename(directory) + " does not contain only directories");
}

template <typename Path>
std::vector<std::string> read_files(const Path& directory)
{
        DirectoryContent content = read_directory(directory);

        if (content.entries.empty())
        {
                error("Files not found in " + generic_utf8_filename(directory));
        }

        if (content.type == ContentType::FILES)
        {
                return std::move(content.entries);
        }

        error("Directory " + generic_utf8_filename(directory) + " does not contain only files");
}

template std::optional<DirectoryInfo> read_directory_info(const std::filesystem::path&);
template std::vector<std::string> read_directories(const std::filesystem::path&);
template std::vector<std::string> read_files(const std::filesystem::path&);
}
