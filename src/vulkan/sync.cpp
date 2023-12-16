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

#include "sync.h"

#include "error.h"

#include <cstdint>

namespace ns::vulkan
{
void reset_fence(const VkDevice device, const VkFence fence)
{
        VULKAN_CHECK(vkResetFences(device, 1, &fence));
}

bool wait_for_fence(const VkDevice device, const VkFence fence, const std::uint64_t timeout_nanoseconds)
{
        static constexpr VkBool32 WAIT_ALL = VK_TRUE;

        const VkResult result = vkWaitForFences(device, 1, &fence, WAIT_ALL, timeout_nanoseconds);

        if (result == VK_SUCCESS)
        {
                return true;
        }

        if (result == VK_TIMEOUT)
        {
                return false;
        }

        VULKAN_ERROR(result);
}
}
