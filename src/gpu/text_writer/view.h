/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <src/text/text_data.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/functionality.h>

#include <vulkan/vulkan_core.h>

#include <memory>

namespace ns::gpu::text_writer
{
class View
{
public:
        static vulkan::DeviceFunctionality device_functionality();

        virtual ~View() = default;

        virtual void set_color(const color::Color& color) const = 0;
        virtual void set_text_size(unsigned size) = 0;

        virtual void create_buffers(RenderBuffers2D* render_buffers, const Region<2, int>& viewport) = 0;
        virtual void delete_buffers() = 0;

        virtual VkSemaphore draw(
                const vulkan::Queue& queue,
                VkSemaphore wait_semaphore,
                unsigned index,
                const text::TextData& text_data) = 0;
};

std::unique_ptr<View> create_view(
        const vulkan::Device* device,
        const vulkan::CommandPool* graphics_command_pool,
        const vulkan::Queue* graphics_queue,
        bool sample_shading,
        const color::Color& color);
}
