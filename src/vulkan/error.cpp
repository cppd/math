/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "strings.h"

#include <src/com/error.h>

#include <vulkan/vulkan_core.h>

#include <sstream>
#include <string>

namespace ns::vulkan::error_implementation
{
namespace
{
[[noreturn]] void vulkan_error(const VkResult code, const std::string& msg)
{
        ASSERT(code != VK_SUCCESS);

        std::ostringstream oss;

        oss << "Vulkan function has failed, return code " << result_to_string(code) << ".";

        if (!msg.empty())
        {
                oss << " " << msg;
        }

        error(oss.str());
}
}

[[noreturn]] void vulkan_function_error(const VkResult code)
{
        vulkan_error(code, {});
}

[[noreturn]] void vulkan_function_error(const VkResult code, const char* const file, const int line)
{
        vulkan_error(code, std::string(file) + ':' + std::to_string(line) + '.');
}
}
