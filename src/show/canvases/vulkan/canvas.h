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
#include "com/matrix.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/instance.h"
#include "graphics/vulkan/render/render_buffer.h"
#include "graphics/vulkan/swapchain.h"

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

struct VulkanCanvas
{
        virtual ~VulkanCanvas() = default;

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
                                    const mat4& matrix, const vulkan::StorageImage& objects) = 0;
        virtual void delete_buffers() = 0;

        virtual bool text_active() const noexcept = 0;
        virtual void draw_text(VkQueue graphics_queue, VkSemaphore wait_semaphore, VkSemaphore finished_semaphore,
                               unsigned image_index, VkFence command_completed_fence, int step_y, int x, int y,
                               const std::vector<std::string>& text) = 0;

        virtual void draw(VkQueue graphics_queue, VkSemaphore wait_semaphore, VkSemaphore finished_semaphore,
                          unsigned image_index, VkFence command_completed_fence) = 0;
};

std::unique_ptr<VulkanCanvas> create_vulkan_canvas(const vulkan::VulkanInstance& instance, bool sample_shading, int text_size);
