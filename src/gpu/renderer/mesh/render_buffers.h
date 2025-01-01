/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/gpu/render_buffers.h>
#include <src/gpu/renderer/buffers/opacity.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

namespace ns::gpu::renderer
{
class RenderBuffers
{
public:
        virtual ~RenderBuffers() = default;

        [[nodiscard]] virtual unsigned width() const = 0;
        [[nodiscard]] virtual unsigned height() const = 0;
        [[nodiscard]] virtual VkSampleCountFlagBits sample_count() const = 0;
        [[nodiscard]] virtual const vulkan::RenderPass& render_pass() const = 0;
        [[nodiscard]] virtual const std::vector<VkFramebuffer>& framebuffers() const = 0;
        [[nodiscard]] virtual const std::vector<VkClearValue>& clear_values() const = 0;
};

std::unique_ptr<RenderBuffers> create_render_buffers(
        const RenderBuffers3D* render_buffers,
        const Opacity& opacity,
        VkDevice device);
}
