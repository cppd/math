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

#include "file_sys.h"

#if 0

std::string file_base_name(const std::string& file_name)
{
        return std::experimental::filesystem::u8path(file_name).filename().u8string();
}

std::string file_parent_path(const std::string& file_name)
{
        return std::experimental::filesystem::u8path(file_name).parent_path().u8string();
}

std::string temp_directory()
{
        return std::experimental::filesystem::temp_directory_path().u8string();
}

#else

#if defined(__linux__)

#include <cstdio>

constexpr const char separators[] = "/";

#elif defined(_WIN32)

#include <windows.h>

constexpr const char separators[] = "\\/";

#else
#error This operation system is not supported
#endif

std::string file_base_name(const std::string& file_name)
{
        size_t n = file_name.find_last_of(separators);
        return (n != std::string::npos) ? file_name.substr(n + 1) : file_name;
}

std::string file_extension(const std::string& file_name)
{
        std::string base_name = file_base_name(file_name);
        size_t n = base_name.find_last_of('.');
        return (n != std::string::npos) ? base_name.substr(n + 1) : "";
}

std::string file_parent_path(const std::string& file_name)
{
        size_t n = file_name.find_last_of(separators);
        return (n != std::string::npos) ? file_name.substr(0, n) : ".";
}

std::string temp_directory()
{
#if defined(__linux__)

        return P_tmpdir;

#elif defined(_WIN32)

        // Тут не UTF8 получается, надо GetTempPathW и затем преобразовать UTF16 в UTF8.
        // Но для открытия файлов функциями типа std::fstream нужно именно это.

        constexpr int buffer_size = MAX_PATH;
        char buf[buffer_size];
        DWORD r = GetTempPathA(buffer_size, buf);
        if (r > buffer_size || r == 0)
        {
                exit(0);
        }
        return buf;

#endif
}

#endif
