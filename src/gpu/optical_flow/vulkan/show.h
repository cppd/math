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

#include <src/graphics/vulkan/buffers.h>
#include <src/graphics/vulkan/instance.h>

#include <memory>
#include <vector>

namespace gpu_vulkan
{
struct OpticalFlowShow
{
        static std::vector<vulkan::PhysicalDeviceFeatures> required_device_features();

        virtual ~OpticalFlowShow() = default;

        virtual void create_buffers(
                RenderBuffers2D* render_buffers,
                const vulkan::ImageWithMemory& input,
                double window_ppi,
                unsigned x,
                unsigned y,
                unsigned width,
                unsigned height) = 0;
        virtual void delete_buffers() = 0;

        virtual VkSemaphore draw(
                const vulkan::Queue& graphics_queue,
                const vulkan::Queue& compute_queue,
                VkSemaphore wait_semaphore,
                unsigned image_index) = 0;

        virtual void reset() = 0;
};

std::unique_ptr<OpticalFlowShow> create_optical_flow_show(
        const vulkan::VulkanInstance& instance,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        bool sample_shading);
}
