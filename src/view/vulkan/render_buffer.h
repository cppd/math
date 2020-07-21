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

#include <src/gpu/buffers.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/swapchain.h>

#include <memory>

namespace view
{
struct RenderBuffers
{
        virtual ~RenderBuffers() = default;

        virtual gpu::RenderBuffers3D& buffers_3d() = 0;
        virtual gpu::RenderBuffers2D& buffers_2d() = 0;

        virtual VkSemaphore resolve_to_swapchain(
                const vulkan::Queue& graphics_queue,
                VkSemaphore swapchain_image_semaphore,
                VkSemaphore wait_semaphore,
                unsigned image_index) const = 0;

        virtual unsigned image_count() const = 0;

        virtual void commands_color_resolve(
                VkCommandBuffer command_buffer,
                const vulkan::ImageWithMemory& image,
                VkImageLayout layout,
                VkPipelineStageFlags src_stage,
                VkPipelineStageFlags dst_stage,
                const Region<2, int>& rectangle,
                unsigned image_index) const = 0;
};

enum class RenderBufferCount
{
        One,
        Swapchain
};

std::unique_ptr<RenderBuffers> create_render_buffers(
        RenderBufferCount buffer_count,
        const vulkan::Swapchain& swapchain,
        const vulkan::CommandPool& command_pool,
        const vulkan::Device& device,
        int required_minimum_sample_count);
}
