/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "render_buffers.h"

#include <src/image/image.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace ns::view::view
{
class ImageResolve final
{
        const std::uint32_t family_index_;
        std::vector<vulkan::ImageWithMemory> images_;
        vulkan::handle::CommandBuffers command_buffers_;

public:
        ImageResolve(
                const vulkan::Device& device,
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                const RenderBuffers& render_buffers,
                const numerical::Region<2, int>& rectangle,
                VkImageLayout image_layout,
                VkImageUsageFlags usage);

        [[nodiscard]] const vulkan::ImageWithMemory& image(unsigned image_index) const;

        void resolve(
                const vulkan::Queue& graphics_queue,
                VkSemaphore wait_semaphore,
                VkSemaphore signal_semaphore,
                unsigned image_index) const;

        void resolve(const vulkan::Queue& graphics_queue, VkSemaphore wait_semaphore, unsigned image_index) const;
};

[[nodiscard]] image::Image<2> resolve_to_image(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const RenderBuffers& render_buffers,
        VkSemaphore wait_semaphore,
        unsigned image_index);
}
