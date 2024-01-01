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

#pragma once

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>

namespace ns::vulkan
{
template <std::size_t N>
void queue_submit(
        const std::array<VkSemaphore, N>& wait_semaphores,
        const std::array<VkPipelineStageFlags, N>& wait_stages,
        VkCommandBuffer command_buffer,
        VkSemaphore signal_semaphore,
        VkQueue queue);

void queue_submit(
        VkSemaphore wait_semaphore,
        VkPipelineStageFlags wait_stage,
        VkCommandBuffer command_buffer,
        VkSemaphore signal_semaphore,
        VkQueue queue);

void queue_submit(
        VkSemaphore wait_semaphore,
        VkPipelineStageFlags wait_stage,
        VkCommandBuffer command_buffer,
        VkQueue queue);

void queue_submit(VkCommandBuffer command_buffer, VkSemaphore signal_semaphore, VkQueue queue);

void queue_submit(VkCommandBuffer command_buffer, VkQueue queue);

void queue_submit(VkQueue queue, VkFence fence);
}
