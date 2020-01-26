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

#include "file_read.h"

#if 0

#include "file.h"

template <typename T>
void read_text_file(const std::string& file_name, T* s)
{
        static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::vector<char>>);

        CFile f(file_name, "rb");

        std::fseek(f, 0, SEEK_END);
        long long length = std::ftell(f);

        if (length == 0)
        {
                s->clear();
                return;
        }

        std::fseek(f, -1, SEEK_END);
        char c;
        std::fscanf(f, "%c", &c);
        if (c == '\n')
        {
                s->resize(length);
        }
        else
        {
                s->resize(length + 1);
                (*s)[s->size() - 1] = '\n';
        }

        std::rewind(f);
        std::fread(s->data(), length, 1, f);
}
#else

#include "com/error.h"

#include <fstream>

template <typename T>
void read_text_file(const std::string& file_name, T* s)
{
        static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::vector<char>>);

        std::ifstream f(file_name, std::ios_base::binary);

        if (!f)
        {
                error("Failed to open file " + file_name);
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
#endif

template void read_text_file(const std::string& file_name, std::string* s);
template void read_text_file(const std::string& file_name, std::vector<char>* s);
