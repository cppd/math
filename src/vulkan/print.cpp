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

#include "print.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>
#include <vulkan/vulkan.h>

namespace ns::vulkan
{
std::string api_version_to_string(const std::uint32_t api_version)
{
        std::ostringstream oss;
        oss << VK_API_VERSION_VARIANT(api_version);
        oss << "." << VK_API_VERSION_MAJOR(api_version);
        oss << "." << VK_API_VERSION_MINOR(api_version);
        oss << "." << VK_API_VERSION_PATCH(api_version);
        return oss.str();
}

std::string strings_to_sorted_string(const std::span<const char* const> strings)
{
        if (strings.empty())
        {
                return {};
        }

        if (strings.size() == 1)
        {
                return strings[0];
        }

        std::vector<const char*> data(strings.begin(), strings.end());

        std::sort(
                data.begin(), data.end(),
                [](const char* const a, const char* const b)
                {
                        return std::strcmp(a, b) < 0;
                });

        std::string s = data[0];
        for (std::size_t i = 1; i < data.size(); ++i)
        {
                s += ", ";
                s += data[i];
        }
        return s;
}
}
