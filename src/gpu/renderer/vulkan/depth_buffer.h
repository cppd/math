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

#pragma once

#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/constant.h"
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"
#include "graphics/vulkan/swapchain.h"

#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

namespace gpu_vulkan
{
struct RendererDepthBuffers
{
        virtual ~RendererDepthBuffers() = default;

        virtual const vulkan::DepthAttachment* texture(unsigned index) const = 0;
        virtual unsigned width() const = 0;
        virtual unsigned height() const = 0;
        virtual VkRenderPass render_pass() const = 0;
        virtual VkSampleCountFlagBits sample_count() const = 0;

        virtual vulkan::CommandBuffers create_command_buffers(const std::function<void(VkCommandBuffer buffer)>& commands) = 0;
};

enum class RendererDepthBufferCount
{
        One,
        Swapchain
};

std::unique_ptr<RendererDepthBuffers> create_renderer_depth_buffers(RendererDepthBufferCount buffer_count,
                                                                    const vulkan::Swapchain& swapchain,
                                                                    const std::unordered_set<uint32_t>& attachment_family_indices,
                                                                    VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                                                                    const vulkan::Device& device, unsigned width, unsigned height,
                                                                    double zoom);
}
