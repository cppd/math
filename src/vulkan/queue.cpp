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

#include "queue.h"

#include "error.h"

#include <array>
#include <cstddef>
#include <vulkan/vulkan_core.h>

namespace ns::vulkan
{
template <std::size_t N>
void queue_submit(
        const std::array<VkSemaphore, N>& wait_semaphores,
        const std::array<VkPipelineStageFlags, N>& wait_stages,
        const VkCommandBuffer command_buffer,
        const VkSemaphore signal_semaphore,
        const VkQueue queue)
{
        VkSubmitInfo info = {};

        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = wait_semaphores.size();
        info.pWaitSemaphores = wait_semaphores.data();
        info.pWaitDstStageMask = wait_stages.data();
        info.commandBufferCount = 1;
        info.pCommandBuffers = &command_buffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &signal_semaphore;

        VULKAN_CHECK(vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE));
}

void queue_submit(
        const VkSemaphore wait_semaphore,
        const VkPipelineStageFlags wait_stage,
        const VkCommandBuffer command_buffer,
        const VkSemaphore signal_semaphore,
        const VkQueue queue)
{
        VkSubmitInfo info = {};

        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &wait_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &command_buffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &signal_semaphore;

        VULKAN_CHECK(vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE));
}

void queue_submit(
        const VkSemaphore wait_semaphore,
        const VkPipelineStageFlags wait_stage,
        const VkCommandBuffer command_buffer,
        const VkQueue queue)
{
        VkSubmitInfo info = {};

        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &wait_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &command_buffer;

        VULKAN_CHECK(vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE));
}

void queue_submit(const VkCommandBuffer command_buffer, const VkSemaphore signal_semaphore, const VkQueue queue)
{
        VkSubmitInfo info = {};

        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &command_buffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &signal_semaphore;

        VULKAN_CHECK(vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE));
}

void queue_submit(const VkCommandBuffer command_buffer, const VkQueue queue)
{
        VkSubmitInfo info = {};

        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &command_buffer;

        VULKAN_CHECK(vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE));
}

void queue_submit(const VkQueue queue, const VkFence fence)
{
        VULKAN_CHECK(vkQueueSubmit(queue, 0, nullptr, fence));
}

template void queue_submit(
        const std::array<VkSemaphore, 2>&,
        const std::array<VkPipelineStageFlags, 2>&,
        VkCommandBuffer,
        VkSemaphore,
        VkQueue);
}
