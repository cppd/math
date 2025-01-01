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

#pragma once

#include <vulkan/vulkan_core.h>

#include <string>
#include <string_view>
#include <vector>

namespace ns::vulkan::strings
{
std::string formats_to_sorted_string(const std::vector<VkFormat>& formats, std::string_view separator);

template <typename T>
std::vector<const char*> strings_to_char_pointers(const T& v)
{
        std::vector<const char*> res;
        res.reserve(std::size(v));
        for (const auto& s : v)
        {
                res.push_back(s.c_str());
        }
        return res;
}

template <typename T>
std::vector<const char*> strings_to_char_pointers(const T&&) = delete;
}
