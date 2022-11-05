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

#include "path.h"

#include <string_view>

namespace ns
{
template <typename T>
std::string generic_utf8_filename(const T& path)
{
        return reinterpret_cast<const char*>(path.generic_u8string().c_str());
}

template <typename T>
std::filesystem::path path_from_utf8(const T& filename)
{
        const auto* const data = reinterpret_cast<const char8_t*>(filename.data());
        return std::filesystem::path(data, data + filename.size());
}

template std::string generic_utf8_filename(const std::filesystem::path&);
template std::filesystem::path path_from_utf8(const std::string&);
template std::filesystem::path path_from_utf8(const std::string_view&);
}
