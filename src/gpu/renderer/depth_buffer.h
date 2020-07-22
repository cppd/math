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

#include <src/vulkan/buffers.h>
#include <src/vulkan/constant.h>
#include <src/vulkan/instance.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>
#include <src/vulkan/swapchain.h>

#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

namespace gpu::renderer
{
struct DepthBuffers
{
        virtual ~DepthBuffers() = default;

        virtual const vulkan::DepthImageWithMemory* texture(unsigned index) const = 0;
        virtual unsigned width() const = 0;
        virtual unsigned height() const = 0;
        virtual VkRenderPass render_pass() const = 0;
        virtual VkSampleCountFlagBits sample_count() const = 0;
        virtual const std::vector<VkFramebuffer>& framebuffers() const = 0;
        virtual const std::vector<VkClearValue>& clear_values() const = 0;
};

enum class DepthBufferCount
{
        One,
        Swapchain
};

std::unique_ptr<DepthBuffers> create_depth_buffers(
        DepthBufferCount buffer_count,
        const vulkan::Swapchain& swapchain,
        const std::unordered_set<uint32_t>& attachment_family_indices,
        VkCommandPool graphics_command_pool,
        VkQueue graphics_queue,
        const vulkan::Device& device,
        unsigned width,
        unsigned height,
        double zoom);
}
