/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <memory>
#include <span>
#include <vector>

namespace ns::view
{
struct RenderBuffers
{
        virtual ~RenderBuffers() = default;

        virtual gpu::RenderBuffers3D& buffers_3d() = 0;
        virtual gpu::RenderBuffers2D& buffers_2d() = 0;

        virtual unsigned width() const = 0;
        virtual unsigned height() const = 0;
        virtual VkFormat color_format() const = 0;
        virtual VkFormat depth_format() const = 0;
        virtual VkSampleCountFlagBits sample_count() const = 0;
        virtual const std::vector<VkImageView>& image_views() const = 0;

        virtual void commands_color_resolve(
                VkCommandBuffer command_buffer,
                VkImage image,
                VkImageLayout layout,
                const Region<2, int>& rectangle,
                unsigned index) const = 0;
};

std::unique_ptr<RenderBuffers> create_render_buffers(
        unsigned buffer_count,
        VkFormat color_format,
        const std::span<const VkFormat>& depth_formats,
        unsigned width,
        unsigned height,
        const std::vector<uint32_t>& family_indices,
        const vulkan::Device& device,
        int required_minimum_sample_count);
}