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

#include "../../vulkan_interfaces.h"

#include <src/color/color.h>
#include <src/graphics/vulkan/instance.h>
#include <src/text/text_data.h>

#include <memory>
#include <vulkan/vulkan.h>

namespace gpu_vulkan
{
struct TextShow
{
        static std::vector<vulkan::PhysicalDeviceFeatures> required_device_features();

        virtual ~TextShow() = default;

        virtual void set_color(const Color& color) const = 0;

        virtual void create_buffers(
                RenderBuffers2D* render_buffers,
                unsigned x,
                unsigned y,
                unsigned width,
                unsigned height) = 0;
        virtual void delete_buffers() = 0;

        virtual VkSemaphore draw(
                const vulkan::Queue& queue,
                VkSemaphore wait_semaphore,
                unsigned image_index,
                const TextData& text_data) = 0;
};

std::unique_ptr<TextShow> create_text_show(
        const vulkan::VulkanInstance& instance,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        bool sample_shading,
        int size,
        const Color& color);
}
