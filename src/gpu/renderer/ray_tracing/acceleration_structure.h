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

#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vector>

namespace ns::gpu::renderer
{
class AccelerationStructure final
{
        vulkan::BufferWithMemory buffer_;
        vulkan::handle::AccelerationStructureKHR acceleration_structure_;
        VkDeviceAddress device_address_;

public:
        AccelerationStructure(
                const vulkan::Device& device,
                vulkan::BufferWithMemory&& buffer,
                vulkan::handle::AccelerationStructureKHR&& handle);

        VkDeviceAddress device_address() const
        {
                return device_address_;
        }
};

AccelerationStructure create_bottom_level_acceleration_structure(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices);

AccelerationStructure create_top_level_acceleration_structure(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        const AccelerationStructure& bottom_level_acceleration_structure);
}
