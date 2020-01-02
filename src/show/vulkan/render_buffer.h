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

#pragma once

#include "gpu/vulkan_interfaces.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/swapchain.h"

#include <memory>

namespace show_vulkan
{
struct RenderBuffers
{
        virtual ~RenderBuffers() = default;

        virtual gpu_vulkan::RenderBuffers3D& buffers_3d() = 0;
        virtual gpu_vulkan::RenderBuffers2D& buffers_2d() = 0;

        virtual VkSemaphore resolve_to_swapchain(const vulkan::Queue& graphics_queue, VkSemaphore swapchain_image_semaphore,
                                                 VkSemaphore wait_semaphore, unsigned image_index) const = 0;

        virtual std::vector<VkImage> images() const = 0;
        virtual VkImageLayout image_layout() const = 0;
};

enum class RenderBufferCount
{
        One,
        Swapchain
};

std::unique_ptr<RenderBuffers> create_render_buffers(RenderBufferCount buffer_count, const vulkan::Swapchain& swapchain,
                                                     const vulkan::CommandPool& command_pool, const vulkan::Device& device,
                                                     int required_minimum_sample_count);
}
