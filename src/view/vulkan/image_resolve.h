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

#include "render_buffers.h"

#include <vector>

namespace ns::view
{
class ImageResolve
{
        const uint32_t family_index_;
        std::vector<vulkan::ImageWithMemory> images_;
        vulkan::CommandBuffers command_buffers_;
        std::vector<vulkan::Semaphore> signal_semaphores_;

public:
        ImageResolve(
                const vulkan::Device& device,
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                const RenderBuffers& render_buffers,
                const Region<2, int>& rectangle,
                VkImageLayout image_layout,
                VkImageUsageFlags usage);

        const vulkan::ImageWithMemory& image(unsigned image_index) const;

        [[nodiscard]] VkSemaphore resolve_semaphore(
                const vulkan::Queue& graphics_queue,
                VkSemaphore wait_semaphore,
                unsigned image_index) const;

        void resolve(const vulkan::Queue& graphics_queue, VkSemaphore wait_semaphore, unsigned image_index) const;
};
}
