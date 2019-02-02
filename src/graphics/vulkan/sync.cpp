/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/type/limit.h"

namespace vulkan
{
void wait_for_fence_and_reset(VkDevice device, VkFence fence)
{
        constexpr VkBool32 wait_all = VK_TRUE;
        constexpr uint64_t timeout = limits<uint64_t>::max();

        VkResult result;

        result = vkWaitForFences(device, 1, &fence, wait_all, timeout);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkWaitForFences", result);
        }

        result = vkResetFences(device, 1, &fence);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkResetFences", result);
        }
}

void queue_wait_idle(VkQueue queue)
{
        VkResult result = vkQueueWaitIdle(queue);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkQueueWaitIdle", result);
        }
}
}
