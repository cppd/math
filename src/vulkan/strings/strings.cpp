/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "strings.h"

#include "format.h"

#include <src/com/string/strings.h>

#include <vulkan/vulkan_core.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ns::vulkan
{
std::string formats_to_sorted_string(const std::vector<VkFormat>& formats, const std::string_view separator)
{
        if (formats.empty())
        {
                return {};
        }

        if (formats.size() == 1)
        {
                return format_to_string(formats.front());
        }

        std::vector<std::string> strings;
        strings.reserve(formats.size());
        for (const VkFormat format : formats)
        {
                strings.push_back(format_to_string(format));
        }

        return strings_to_sorted_string(std::move(strings), separator);
}
}
