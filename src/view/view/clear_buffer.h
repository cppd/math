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

#include "render_buffers.h"

#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

namespace ns::view::view
{
class ClearBuffer final
{
        VkDevice device_;
        VkCommandPool graphics_command_pool_;
        vulkan::handle::Semaphore clear_semaphore_;

        const RenderBuffers* render_buffers_ = nullptr;
        const vulkan::ImageWithMemory* image_ = nullptr;

        vulkan::handle::CommandBuffers command_buffers_;

public:
        ClearBuffer(VkDevice device, VkCommandPool graphics_command_pool);

        void create_buffers(
                const RenderBuffers* render_buffers,
                const vulkan::ImageWithMemory* image,
                const numerical::Vector3f& clear_color);

        void delete_buffers();

        [[nodiscard]] VkSemaphore clear(const vulkan::Queue& graphics_queue, unsigned index) const;

        void set_color(const numerical::Vector3f& clear_color);
};
}
