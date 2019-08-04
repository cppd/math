/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/color/color.h"
#include "com/font/text_data.h"
#include "com/matrix.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/render/render_buffer.h"
#include "graphics/vulkan/swapchain.h"

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace gpu_vulkan
{
struct Canvas
{
        virtual ~Canvas() = default;

        virtual void set_text_color(const Color& c) = 0;
        virtual void set_text_active(bool v) = 0;

        virtual void set_pencil_sketch_active(bool v) = 0;
        virtual bool pencil_sketch_active() = 0;

        virtual void set_dft_active(bool v) = 0;
        virtual bool dft_active() = 0;
        virtual void set_dft_brightness(double v) = 0;
        virtual void set_dft_background_color(const Color& c) = 0;
        virtual void set_dft_color(const Color& c) = 0;

        virtual void set_convex_hull_active(bool v) = 0;

        virtual void set_optical_flow_active(bool v) = 0;

        //

        static std::vector<vulkan::PhysicalDeviceFeatures> required_device_features();

        virtual void create_buffers(const vulkan::Swapchain* swapchain, vulkan::RenderBuffers2D* render_buffers,
                                    const mat4& matrix, const vulkan::StorageImage* objects) = 0;
        virtual void delete_buffers() = 0;

        virtual VkSemaphore draw(const vulkan::Queue& graphics_queue, const vulkan::Queue& graphics_compute_queue,
                                 VkSemaphore wait_semaphore, unsigned image_index, const TextData& text_data) = 0;
};

std::unique_ptr<Canvas> create_canvas(const vulkan::VulkanInstance& instance, const vulkan::CommandPool& graphics_command_pool,
                                      const vulkan::Queue& graphics_queue, const vulkan::CommandPool& transfer_command_pool,
                                      const vulkan::Queue& transfer_queue, const vulkan::Queue& graphics_compute_queue,
                                      bool sample_shading, int text_size);
}
