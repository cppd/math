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

#include "read.h"

#include "path.h"

#include <src/com/error.h>

#include <fstream>
#include <ios>
#include <vector>

namespace ns
{
template <typename Path>
std::vector<char> read_file(const Path& path)
{
        std::ifstream f(path, std::ios_base::binary);

        if (!f)
        {
                error("Failed to open file " + generic_utf8_filename(path));
        }

        f.seekg(0, std::ios_base::end);
        const long long length = f.tellg();

        std::vector<char> res(length);

        if (length == 0)
        {
                if (!f)
                {
                        error("Failed to read file " + generic_utf8_filename(path));
                }

                return res;
        }

        f.seekg(0, std::ios_base::beg);
        f.read(res.data(), length);

        if (!f)
        {
                error("Failed to read file " + generic_utf8_filename(path));
        }

        return res;
}

template std::vector<char> read_file(const std::filesystem::path&);
}
