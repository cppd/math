/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "read.h"

#include "path.h"

#include "../error.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace ns
{
template <typename T, typename Path>
T read_text_file(const Path& path)
{
        static_assert(sizeof(typename T::value_type) == 1);

        std::ifstream f(path, std::ios_base::binary);

        if (!f)
        {
                error("Failed to open file " + generic_utf8_filename(path));
        }

        f.seekg(0, std::ios_base::end);
        const long long length = f.tellg();

        if (length == 0)
        {
                return {};
        }

        T res;

        f.seekg(-1, std::ios_base::end);
        if (f.get() == '\n')
        {
                res.resize(length);
        }
        else
        {
                res.resize(length + 1);
                res.back() = '\n';
        }

        f.seekg(0, std::ios_base::beg);
        f.read(res.data(), length);

        return res;
}

template <typename T, typename Path>
T read_binary_file(const Path& path)
{
        static_assert(sizeof(typename T::value_type) == 1);

        std::ifstream f(path, std::ios_base::binary);

        if (!f)
        {
                error("Failed to open file " + generic_utf8_filename(path));
        }

        f.seekg(0, std::ios_base::end);
        const long long length = f.tellg();

        if (length == 0)
        {
                return {};
        }

        T res;

        res.resize(length);
        f.seekg(0, std::ios_base::beg);
        f.read(res.data(), length);

        return res;
}

template std::string read_text_file(const std::filesystem::path&);
template std::vector<char> read_text_file(const std::filesystem::path&);

template std::string read_binary_file(const std::filesystem::path&);
template std::vector<char> read_binary_file(const std::filesystem::path&);
}
