/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <fstream>

namespace ns
{
template <typename T>
void read_text_file(const std::filesystem::path& file_name, T* s)
{
        static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::vector<char>>);

        std::ifstream f(file_name, std::ios_base::binary);

        if (!f)
        {
                error("Failed to open file " + generic_utf8_filename(file_name));
        }

        f.seekg(0, std::ios_base::end);
        long long length = f.tellg();
        if (length == 0)
        {
                s->clear();
                return;
        }

        f.seekg(-1, std::ios_base::end);
        if (f.get() == '\n')
        {
                s->resize(length);
        }
        else
        {
                s->resize(length + 1);
                (*s)[s->size() - 1] = '\n';
        }

        f.seekg(0, std::ios_base::beg);
        f.read(s->data(), length);
}

template <typename T>
void read_binary_file(const std::filesystem::path& file_name, T* s)
{
        static_assert(sizeof(typename T::value_type) == 1);

        std::ifstream f(file_name, std::ios_base::binary);

        if (!f)
        {
                error("Failed to open file " + generic_utf8_filename(file_name));
        }

        f.seekg(0, std::ios_base::end);
        long long length = f.tellg();

        if (length == 0)
        {
                s->clear();
                return;
        }

        s->resize(length);
        f.seekg(0, std::ios_base::beg);
        f.read(s->data(), length);
}

template void read_text_file(const std::filesystem::path&, std::string*);
template void read_text_file(const std::filesystem::path&, std::vector<char>*);

template void read_binary_file(const std::filesystem::path&, std::string*);
template void read_binary_file(const std::filesystem::path&, std::vector<char>*);
}
