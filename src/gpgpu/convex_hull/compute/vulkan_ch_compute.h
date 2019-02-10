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

#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/instance.h"

#include <memory>

namespace gpgpu_vulkan
{
struct ConvexHullCompute
{
        static std::vector<vulkan::PhysicalDeviceFeatures> required_device_features();

        virtual ~ConvexHullCompute() = default;

        virtual void compute_commands(VkCommandBuffer command_buffer) const = 0;
        virtual void create_buffers(const vulkan::StorageImage& objects,
                                    const vulkan::StorageBufferWithHostVisibleMemory& points_buffer,
                                    const vulkan::IndirectBufferWithHostVisibleMemory& point_count_buffer) = 0;
        virtual void delete_buffers() = 0;
};

std::unique_ptr<ConvexHullCompute> create_convex_hull_compute(const vulkan::VulkanInstance& instance);
}
