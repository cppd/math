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

#include "extensions.h"

#include <src/com/error.h>

namespace ns::vulkan
{
PFN_vkVoidFunction proc_addr(const VkInstance instance, const char* const name)
{
        ASSERT(instance != VK_NULL_HANDLE);
        const PFN_vkVoidFunction addr = vkGetInstanceProcAddr(instance, name);
        if (addr != nullptr)
        {
                return addr;
        }
        error(std::string("Failed to find address of ").append(name));
}
}
