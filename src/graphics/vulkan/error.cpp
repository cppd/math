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

#include "error.h"

#include "print.h"

#include "com/error.h"

namespace
{
std::string return_code_string(const std::string& function_name, const VkResult& code)
{
        std::array<std::string, 2> strings = vulkan::result_to_strings(code);

        std::string result;

        if (function_name.size() > 0)
        {
                result += function_name + ".";
        }

        for (const std::string& s : strings)
        {
                if (s.size() > 0)
                {
                        result += " " + s + ".";
                }
        }

        if (result.size() > 0)
        {
                return result;
        }

        error("Vulkan no return code information");
}
}

namespace vulkan
{
[[noreturn]] void vulkan_function_error(const std::string& function_name, const VkResult& code)
{
        error("Vulkan Error. " + return_code_string(function_name, code));
}
}
