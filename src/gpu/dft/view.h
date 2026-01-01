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

#include <src/color/color.h>
#include <src/gpu/render_buffers.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/functionality.h>

#include <vulkan/vulkan_core.h>

#include <memory>

namespace ns::gpu::dft
{
class View
{
public:
        static vulkan::physical_device::DeviceFunctionality device_functionality();

        virtual ~View() = default;

        virtual void create_buffers(
                RenderBuffers2D* render_buffers,
                const vulkan::ImageWithMemory& input,
                const numerical::Region<2, int>& source_rectangle,
                const numerical::Region<2, int>& draw_rectangle) = 0;

        virtual void delete_buffers() = 0;

        virtual VkSemaphore draw(const vulkan::Queue& queue, VkSemaphore wait_semaphore, unsigned index) const = 0;

        virtual void set_brightness(double brightness) = 0;
        virtual void set_background_color(const color::Color& color) = 0;
        virtual void set_color(const color::Color& color) = 0;
};

std::unique_ptr<View> create_view(
        const vulkan::Device* device,
        const vulkan::CommandPool* graphics_command_pool,
        const vulkan::Queue* graphics_queue,
        const vulkan::CommandPool* transfer_command_pool,
        const vulkan::Queue* transfer_queue);
}
