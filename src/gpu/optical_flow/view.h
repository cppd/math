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
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device_instance.h>
#include <src/vulkan/functionality.h>

#include <memory>

namespace ns::gpu::optical_flow
{
struct View
{
        static vulkan::DeviceFunctionality device_functionality();

        virtual ~View() = default;

        virtual void create_buffers(
                RenderBuffers2D* render_buffers,
                const vulkan::ImageWithMemory& input,
                double window_ppi,
                const Region<2, int>& rectangle) = 0;
        virtual void delete_buffers() = 0;

        virtual VkSemaphore draw(
                const vulkan::Queue& graphics_queue,
                const vulkan::Queue& compute_queue,
                VkSemaphore wait_semaphore,
                unsigned index) const = 0;

        virtual void reset() = 0;
};

std::unique_ptr<View> create_view(
        const vulkan::DeviceInstance* instance,
        const vulkan::CommandPool* graphics_command_pool,
        const vulkan::Queue* graphics_queue,
        const vulkan::CommandPool* compute_command_pool,
        const vulkan::Queue* compute_queue,
        const vulkan::CommandPool* transfer_command_pool,
        const vulkan::Queue* transfer_queue,
        bool sample_shading);
}
