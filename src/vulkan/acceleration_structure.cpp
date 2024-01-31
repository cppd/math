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

#include "acceleration_structure.h"

#include "buffers.h"
#include "commands.h"
#include "extensions.h"
#include "objects.h"

#include "device/device.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace ns::vulkan
{
namespace
{
VkDeviceAddress acceleration_structure_device_address(
        const VkDevice device,
        const VkAccelerationStructureKHR acceleration_structure)
{
        VkAccelerationStructureDeviceAddressInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        info.accelerationStructure = acceleration_structure;
        return vkGetAccelerationStructureDeviceAddressKHR(device, &info);
}

VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes(
        const VkDevice device,
        const VkAccelerationStructureGeometryKHR& geometry,
        const VkAccelerationStructureTypeKHR type,
        const std::uint32_t primitive_count,
        const bool allow_update)
{
        VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info{};
        build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_geometry_info.type = type;
        build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        if (allow_update)
        {
                build_geometry_info.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        }
        build_geometry_info.geometryCount = 1;
        build_geometry_info.pGeometries = &geometry;

        VkAccelerationStructureBuildSizesInfoKHR build_sizes_info{};
        build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        vkGetAccelerationStructureBuildSizesKHR(
                device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_geometry_info, &primitive_count,
                &build_sizes_info);

        return build_sizes_info;
}

void build_acceleration_structure(
        const VkDevice device,
        const CommandPool& compute_command_pool,
        const Queue& compute_queue,
        const BufferWithMemory& scratch_buffer,
        const VkAccelerationStructureGeometryKHR& geometry,
        const VkAccelerationStructureTypeKHR type,
        const VkBuildAccelerationStructureModeKHR mode,
        const VkAccelerationStructureKHR acceleration_structure,
        const std::uint32_t primitive_count,
        const bool allow_update)
{
        VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info{};
        build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_geometry_info.type = type;
        build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        if (allow_update)
        {
                build_geometry_info.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        }
        build_geometry_info.mode = mode;
        if (mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR)
        {
                build_geometry_info.srcAccelerationStructure = acceleration_structure;
        }
        build_geometry_info.dstAccelerationStructure = acceleration_structure;
        build_geometry_info.geometryCount = 1;
        build_geometry_info.pGeometries = &geometry;
        build_geometry_info.scratchData.deviceAddress = scratch_buffer.device_address();

        VkAccelerationStructureBuildRangeInfoKHR build_range_info{};
        build_range_info.primitiveCount = primitive_count;
        build_range_info.primitiveOffset = 0;
        build_range_info.firstVertex = 0;
        build_range_info.transformOffset = 0;

        const std::vector<VkAccelerationStructureBuildRangeInfoKHR*> build_range_infos = {&build_range_info};

        const auto commands = [&](const VkCommandBuffer command_buffer)
        {
                vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &build_geometry_info, build_range_infos.data());
        };

        run_commands(device, compute_command_pool.handle(), compute_queue.handle(), commands);
}

BufferWithMemory create_vertex_buffer(
        const Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const std::span<const numerical::Vector3f> vertices)
{
        constexpr VkBufferUsageFlags USAGE =
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

        BufferWithMemory res(BufferMemoryType::HOST_VISIBLE, device, family_indices, USAGE, data_size(vertices));
        BufferMapper(res).write(vertices);
        return res;
}

BufferWithMemory create_index_buffer(
        const Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const std::span<const std::uint32_t> indices)
{
        constexpr VkBufferUsageFlags USAGE =
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

        BufferWithMemory res(BufferMemoryType::HOST_VISIBLE, device, family_indices, USAGE, data_size(indices));
        BufferMapper(res).write(indices);
        return res;
}

std::optional<BufferWithMemory> create_transform_matrix_buffer(
        const Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const std::optional<VkTransformMatrixKHR>& transform_matrix)
{
        if (!transform_matrix)
        {
                return {};
        }

        constexpr VkBufferUsageFlags USAGE =
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

        std::optional<BufferWithMemory> res;
        res.emplace(BufferMemoryType::HOST_VISIBLE, device, family_indices, USAGE, data_size(*transform_matrix));
        BufferMapper(*res).write(*transform_matrix);
        return res;
}

void check_data(const std::span<const numerical::Vector3f> vertices, const std::span<const std::uint32_t> indices)
{
        if (vertices.empty())
        {
                error("No vertices for acceleration structure");
        }

        if (indices.empty())
        {
                error("No indices for acceleration structure");
        }

        if (!(indices.size() % 3 == 0))
        {
                error("Index count " + to_string(indices.size()) + " is not a multiple of 3");
        }
}

void check_data(
        const std::span<const std::uint64_t> bottom_level_references,
        const std::span<const VkTransformMatrixKHR> bottom_level_matrices)
{
        if (bottom_level_references.size() != bottom_level_matrices.size())
        {
                error("Bottom level reference count " + to_string(bottom_level_references.size())
                      + " is not equal to matrix count " + to_string(bottom_level_matrices.size()));
        }
}
}

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(
        BufferWithMemory&& buffer,
        handle::AccelerationStructureKHR&& handle)
        : buffer_(std::move(buffer)),
          acceleration_structure_(std::move(handle)),
          device_address_(acceleration_structure_device_address(buffer_.buffer().device(), acceleration_structure_))
{
}

TopLevelAccelerationStructure::TopLevelAccelerationStructure(
        BufferWithMemory&& buffer,
        handle::AccelerationStructureKHR&& handle,
        const VkAccelerationStructureGeometryKHR& geometry,
        const std::uint32_t geometry_primitive_count,
        BufferWithMemory&& instance_buffer,
        BufferWithMemory&& scratch_buffer_update)
        : buffer_(std::move(buffer)),
          acceleration_structure_(std::move(handle)),
          device_address_(acceleration_structure_device_address(buffer_.buffer().device(), acceleration_structure_)),
          geometry_(geometry),
          geometry_primitive_count_(geometry_primitive_count),
          instance_buffer_(std::move(instance_buffer)),
          scratch_buffer_update_(std::move(scratch_buffer_update))
{
}

void TopLevelAccelerationStructure::update_matrices(
        const VkDevice device,
        const CommandPool& compute_command_pool,
        const Queue& compute_queue,
        const std::span<const VkTransformMatrixKHR> bottom_level_matrices) const
{
        if (bottom_level_matrices.size() != geometry_primitive_count_)
        {
                error("Bottom level matrix count " + to_string(bottom_level_matrices.size()) + " is not equal to "
                      + to_string(geometry_primitive_count_));
        }

        {
                constexpr std::size_t SIZE = sizeof(VkAccelerationStructureInstanceKHR);
                constexpr std::size_t OFFSET = offsetof(VkAccelerationStructureInstanceKHR, transform);
                const BufferMapper mapper(instance_buffer_);
                for (std::size_t i = 0; i < geometry_primitive_count_; ++i)
                {
                        mapper.write(i * SIZE + OFFSET, bottom_level_matrices[i]);
                }
        }

        constexpr bool ALLOW_UPDATE = true;

        build_acceleration_structure(
                device, compute_command_pool, compute_queue, scratch_buffer_update_, geometry_,
                VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR,
                acceleration_structure_, geometry_primitive_count_, ALLOW_UPDATE);
}

BottomLevelAccelerationStructure create_bottom_level_acceleration_structure(
        const Device& device,
        const CommandPool& compute_command_pool,
        const Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        const std::span<const numerical::Vector3f> vertices,
        const std::span<const std::uint32_t> indices,
        const std::optional<VkTransformMatrixKHR>& transform_matrix)
{
        check_data(vertices, indices);

        const std::uint32_t geometry_primitive_count = indices.size() / 3;

        const std::vector<std::uint32_t> buffer_family_indices = {compute_queue.family_index()};

        const BufferWithMemory vertex_buffer = create_vertex_buffer(device, buffer_family_indices, vertices);

        const BufferWithMemory index_buffer = create_index_buffer(device, buffer_family_indices, indices);

        const std::optional<BufferWithMemory> transform_matrix_buffer =
                create_transform_matrix_buffer(device, buffer_family_indices, transform_matrix);

        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometry.geometry.triangles.vertexData.deviceAddress = vertex_buffer.device_address();
        geometry.geometry.triangles.maxVertex = vertices.size() - 1;
        geometry.geometry.triangles.vertexStride = sizeof(vertices[0]);
        geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        geometry.geometry.triangles.indexData.deviceAddress = index_buffer.device_address();
        if (transform_matrix_buffer)
        {
                geometry.geometry.triangles.transformData.deviceAddress = transform_matrix_buffer->device_address();
        }

        constexpr bool ALLOW_UPDATE = false;

        const VkAccelerationStructureBuildSizesInfoKHR build_sizes = acceleration_structure_build_sizes(
                device.handle(), geometry, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, geometry_primitive_count,
                ALLOW_UPDATE);

        BufferWithMemory acceleration_structure_buffer(
                BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                build_sizes.accelerationStructureSize);

        VkAccelerationStructureCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        create_info.buffer = acceleration_structure_buffer.buffer().handle();
        create_info.size = build_sizes.accelerationStructureSize;
        create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        handle::AccelerationStructureKHR acceleration_structure(device.handle(), create_info);

        {
                const BufferWithMemory scratch_buffer(
                        BufferMemoryType::DEVICE_LOCAL, device, buffer_family_indices,
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        build_sizes.buildScratchSize);

                build_acceleration_structure(
                        device.handle(), compute_command_pool, compute_queue, scratch_buffer, geometry,
                        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
                        acceleration_structure, geometry_primitive_count, ALLOW_UPDATE);
        }

        return {std::move(acceleration_structure_buffer), std::move(acceleration_structure)};
}

TopLevelAccelerationStructure create_top_level_acceleration_structure(
        const Device& device,
        const CommandPool& compute_command_pool,
        const Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        const std::span<const std::uint64_t> bottom_level_references,
        const std::span<const VkTransformMatrixKHR> bottom_level_matrices)
{
        constexpr std::size_t MIN_BUFFER_SIZE = 1;

        check_data(bottom_level_references, bottom_level_matrices);

        const std::uint32_t geometry_primitive_count = bottom_level_references.size();

        const std::vector<std::uint32_t> buffer_family_indices = {compute_queue.family_index()};

        std::vector<VkAccelerationStructureInstanceKHR> instances(bottom_level_references.size());
        for (std::size_t i = 0; i < bottom_level_references.size(); ++i)
        {
                instances[i] = {};
                instances[i].transform = bottom_level_matrices[i];
                instances[i].instanceCustomIndex = 0;
                instances[i].mask = 0xFF;
                instances[i].instanceShaderBindingTableRecordOffset = 0;
                instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
                instances[i].accelerationStructureReference = bottom_level_references[i];
        }

        BufferWithMemory instance_buffer(
                BufferMemoryType::HOST_VISIBLE, device, buffer_family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                std::max<std::size_t>(MIN_BUFFER_SIZE, data_size(instances)));
        BufferMapper(instance_buffer).write(instances);

        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometry.geometry.instances.arrayOfPointers = VK_FALSE;
        geometry.geometry.instances.data.deviceAddress = instance_buffer.device_address();

        constexpr bool ALLOW_UPDATE = true;

        const VkAccelerationStructureBuildSizesInfoKHR build_sizes = acceleration_structure_build_sizes(
                device.handle(), geometry, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, geometry_primitive_count,
                ALLOW_UPDATE);

        BufferWithMemory acceleration_structure_buffer(
                BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                std::max<std::size_t>(MIN_BUFFER_SIZE, build_sizes.accelerationStructureSize));

        VkAccelerationStructureCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        create_info.buffer = acceleration_structure_buffer.buffer().handle();
        create_info.size = build_sizes.accelerationStructureSize;
        create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

        handle::AccelerationStructureKHR acceleration_structure(device.handle(), create_info);

        {
                const BufferWithMemory scratch_buffer_build(
                        BufferMemoryType::DEVICE_LOCAL, device, buffer_family_indices,
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        std::max<std::size_t>(MIN_BUFFER_SIZE, build_sizes.buildScratchSize));

                build_acceleration_structure(
                        device.handle(), compute_command_pool, compute_queue, scratch_buffer_build, geometry,
                        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
                        acceleration_structure, geometry_primitive_count, ALLOW_UPDATE);
        }

        BufferWithMemory scratch_buffer_update(
                BufferMemoryType::DEVICE_LOCAL, device, buffer_family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                std::max<std::size_t>(MIN_BUFFER_SIZE, build_sizes.updateScratchSize));

        return {std::move(acceleration_structure_buffer),
                std::move(acceleration_structure),
                geometry,
                geometry_primitive_count,
                std::move(instance_buffer),
                std::move(scratch_buffer_update)};
}
}
