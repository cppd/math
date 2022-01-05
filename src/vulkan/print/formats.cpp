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

#include "formats.h"

#include "format.h"

#include <src/com/alg.h>

namespace ns::vulkan
{
std::string formats_to_sorted_string(const std::vector<VkFormat>& formats, const std::string_view& separator)
{
        if (formats.empty())
        {
                return {};
        }

        std::vector<std::string> strings;
        strings.reserve(formats.size());
        for (const VkFormat format : formats)
        {
                strings.push_back(format_to_string(format));
        }

        sort_and_unique(&strings);

        auto iter = strings.cbegin();
        std::string s = *iter;
        while (++iter != strings.cend())
        {
                s += separator;
                s += *iter;
        }
        return s;
}
}
