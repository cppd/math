/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "error.h"

#include "print.h"

#include <src/com/error.h>

namespace ns::vulkan
{
[[noreturn]] void vulkan_function_error(const std::string_view& function_name, VkResult code)
{
        std::string result;

        if (!function_name.empty())
        {
                result = function_name;
        }
        else
        {
                result = "Vulkan function";
        }
        result += " has failed.";

        for (const std::string& s : vulkan::result_to_strings(code))
        {
                if (!s.empty())
                {
                        result += ' ';
                        result += s;
                        result += '.';
                }
        }

        error(result);
}
}
