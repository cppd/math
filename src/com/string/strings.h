/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/alg.h>

#include <string>
#include <string_view>
#include <vector>

namespace ns
{
namespace strings_implementation
{
template <typename T>
        requires (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
std::string strings_to_sorted_string(std::vector<T>&& strings, const std::string_view separator)
{
        sort_and_unique(&strings);

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

template <typename T>
        requires (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
std::string strings_to_sorted_string(std::vector<T>&& strings, const std::string_view separator)
{
        if (strings.empty())
        {
                return {};
        }

        if (strings.size() == 1)
        {
                return std::string(std::move(strings.front()));
        }

        return strings_implementation::strings_to_sorted_string(std::move(strings), separator);
}

template <typename T>
std::string strings_to_sorted_string(const T& strings, const std::string_view separator)
{
        if (strings.empty())
        {
                return {};
        }

        if (strings.size() == 1)
        {
                return std::string(*std::cbegin(strings));
        }

        return strings_implementation::strings_to_sorted_string(
                std::vector<std::string_view>(std::cbegin(strings), std::cend(strings)), separator);
}
}
