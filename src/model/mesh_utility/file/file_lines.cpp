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

#include "file_lines.h"

#include <src/com/error.h>
#include <src/utility/file/read.h>

namespace mesh::file
{
namespace
{
template <typename T>
void find_line_begin(const T& s, std::vector<long long>* line_begin)
{
        static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::vector<char>>);

        long long size = s.size();

        long long new_line_count = 0;
        for (long long i = 0; i < size; ++i)
        {
                if (s[i] == '\n')
                {
                        ++new_line_count;
                }
        }

        line_begin->clear();
        line_begin->reserve(new_line_count);
        line_begin->shrink_to_fit();

        long long begin = 0;
        for (long long i = 0; i < size; ++i)
        {
                if (s[i] == '\n')
                {
                        line_begin->push_back(begin);
                        begin = i + 1;
                }
        }

        if (begin != size)
        {
                error("No new line at the end of file");
        }
}
}

template <typename T>
void read_file_lines(const std::string& file_name, T* file_data, std::vector<long long>* line_begin)
{
        static_assert(std::is_same_v<T, std::string> || std::is_same_v<T, std::vector<char>>);

        read_text_file(file_name, file_data);

        find_line_begin(*file_data, line_begin);
}

//

template void read_file_lines(const std::string& file_name, std::string* file_data, std::vector<long long>* line_begin);

template void read_file_lines(
        const std::string& file_name,
        std::vector<char>* file_data,
        std::vector<long long>* line_begin);
}
