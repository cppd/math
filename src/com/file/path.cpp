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

#include "path.h"

#include <string_view>
#include <version>

template <typename T>
std::string generic_utf8_filename(const T& path)
{
        return reinterpret_cast<const char*>(path.generic_u8string().c_str());
}

#if defined(__cpp_lib_char8_t) && __cpp_lib_char8_t >= 201907L
template <typename T>
std::filesystem::path path_from_utf8(const T& filename)
{
        const char8_t* data = reinterpret_cast<const char8_t*>(filename.data());
        return std::filesystem::path(data, data + filename.size());
}
#else
#if !defined(__clang__)
#error __cpp_lib_char8_t
#endif
template <typename T>
std::filesystem::path path_from_utf8(const T& filename)
{
        return std::filesystem::u8path(filename);
}
#endif

template std::string generic_utf8_filename(const std::filesystem::path& path);
template std::filesystem::path path_from_utf8(const std::string& filename);
template std::filesystem::path path_from_utf8(const std::string_view& filename);
