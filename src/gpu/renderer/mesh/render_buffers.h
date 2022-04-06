/*
Copyright (C) 2017-2022 Topological Manifold

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
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <memory>
#include <vector>

namespace ns::gpu::renderer
{
struct RenderBuffers
{
        virtual ~RenderBuffers() = default;

        virtual VkRenderPass render_pass() const = 0;
        virtual const std::vector<VkFramebuffer>& framebuffers() const = 0;
};

std::unique_ptr<RenderBuffers> create_render_buffers(RenderBuffers3D* render_buffers, const vulkan::Device& device);
}
