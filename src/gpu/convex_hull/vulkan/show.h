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

#include "gpu/vulkan_interfaces.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/instance.h"

#include <memory>
#include <vector>

namespace gpu_vulkan
{
struct ConvexHullShow
{
        static std::vector<vulkan::PhysicalDeviceFeatures> required_device_features();

        virtual ~ConvexHullShow() = default;

        virtual void create_buffers(RenderBuffers2D* render_buffers, const vulkan::ImageWithMemory& objects, unsigned x,
                                    unsigned y, unsigned width, unsigned height) = 0;
        virtual void delete_buffers() = 0;

        virtual VkSemaphore draw(const vulkan::Queue& queue, VkSemaphore wait_semaphore, unsigned image_index) = 0;

        virtual void reset_timer() = 0;
};

std::unique_ptr<ConvexHullShow> create_convex_hull_show(const vulkan::VulkanInstance& instance, uint32_t family_index,
                                                        bool sample_shading);
}
