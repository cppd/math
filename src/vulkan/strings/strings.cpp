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

#include "strings.h"

#include "format.h"

#include <src/com/alg.h>
#include <src/com/error.h>

namespace ns::vulkan
{
namespace
{
template <typename T>
std::string sort_strings(std::vector<T>&& strings, const std::string_view separator)
{
        sort_and_unique(&strings);

        ASSERT(!strings.empty());

        auto iter = strings.cbegin();
        std::string res{*iter};
        while (++iter != strings.cend())
        {
                res += separator;
                res += *iter;
        }
        return res;
}
}

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

        return sort_strings(std::move(strings), separator);
}

template <typename T>
std::string strings_to_sorted_string(const std::vector<T>& strings, const std::string_view separator)
{
        if (strings.empty())
        {
                return {};
        }

        if (strings.size() == 1)
        {
                return strings.front();
        }

        return sort_strings(std::vector<std::string_view>(strings.cbegin(), strings.cend()), separator);
}

template std::string strings_to_sorted_string(const std::vector<const char*>&, std::string_view);
template std::string strings_to_sorted_string(const std::vector<std::string>&, std::string_view);
}
