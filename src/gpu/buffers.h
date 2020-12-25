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

#include <src/color/color.h>
#include <src/numerical/region.h>

#include <vector>
#include <vulkan/vulkan.h>

namespace ns::gpu
{
class RenderBuffers3D
{
protected:
        virtual ~RenderBuffers3D() = default;

public:
        virtual unsigned width() const = 0;
        virtual unsigned height() const = 0;
        virtual VkSampleCountFlagBits sample_count() const = 0;
        virtual VkFormat depth_format() const = 0;

        virtual VkRenderPass render_pass() const = 0;
        virtual const std::vector<VkFramebuffer>& framebuffers() const = 0;
        virtual VkRenderPass render_pass_clear() const = 0;
        virtual const std::vector<VkFramebuffer>& framebuffers_clear() const = 0;

        virtual std::vector<VkClearValue> clear_values(const Color& clear_color) const = 0;

        virtual void commands_depth_copy(
                VkCommandBuffer command_buffer,
                VkImage image,
                VkImageLayout layout,
                const Region<2, int>& rectangle,
                unsigned image_index) const = 0;
};

class RenderBuffers2D
{
protected:
        virtual ~RenderBuffers2D() = default;

public:
        virtual unsigned width() const = 0;
        virtual unsigned height() const = 0;
        virtual VkSampleCountFlagBits sample_count() const = 0;
        virtual VkRenderPass render_pass() const = 0;
        virtual const std::vector<VkFramebuffer>& framebuffers() const = 0;
};
}
