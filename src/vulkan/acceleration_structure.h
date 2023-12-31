/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "buffers.h"
#include "objects.h"

#include "device/device.h"

#include <src/numerical/vector.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ns::vulkan
{
class BottomLevelAccelerationStructure final
{
        BufferWithMemory buffer_;
        handle::AccelerationStructureKHR acceleration_structure_;
        VkDeviceAddress device_address_;

public:
        BottomLevelAccelerationStructure(BufferWithMemory&& buffer, handle::AccelerationStructureKHR&& handle);

        [[nodiscard]] VkAccelerationStructureKHR handle() const
        {
                return acceleration_structure_;
        }

        [[nodiscard]] VkDeviceAddress device_address() const
        {
                return device_address_;
        }
};

class TopLevelAccelerationStructure final
{
        BufferWithMemory buffer_;
        handle::AccelerationStructureKHR acceleration_structure_;
        VkDeviceAddress device_address_;

        VkAccelerationStructureGeometryKHR geometry_;
        std::uint32_t geometry_primitive_count_;
        BufferWithMemory instance_buffer_;
        BufferWithMemory scratch_buffer_update_;

public:
        TopLevelAccelerationStructure(
                BufferWithMemory&& buffer,
                handle::AccelerationStructureKHR&& handle,
                const VkAccelerationStructureGeometryKHR& geometry,
                std::uint32_t geometry_primitive_count,
                BufferWithMemory&& instance_buffer,
                BufferWithMemory&& scratch_buffer_update);

        [[nodiscard]] VkAccelerationStructureKHR handle() const
        {
                return acceleration_structure_;
        }

        [[nodiscard]] VkDeviceAddress device_address() const
        {
                return device_address_;
        }

        void update_matrices(
                VkDevice device,
                const CommandPool& compute_command_pool,
                const Queue& compute_queue,
                std::span<const VkTransformMatrixKHR> bottom_level_matrices) const;
};

BottomLevelAccelerationStructure create_bottom_level_acceleration_structure(
        const Device& device,
        const CommandPool& compute_command_pool,
        const Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        std::span<const Vector3f> vertices,
        std::span<const std::uint32_t> indices,
        const std::optional<VkTransformMatrixKHR>& transform_matrix);

TopLevelAccelerationStructure create_top_level_acceleration_structure(
        const Device& device,
        const CommandPool& compute_command_pool,
        const Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        std::span<const std::uint64_t> bottom_level_references,
        std::span<const VkTransformMatrixKHR> bottom_level_matrices);
}
