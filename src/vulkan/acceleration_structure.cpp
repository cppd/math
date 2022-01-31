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

#include "acceleration_structure.h"

#include "error.h"
#include "extensions.h"
#include "queue.h"

#include <src/com/container.h>
#include <src/com/print.h>

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

void begin_commands(const VkCommandBuffer command_buffer)
{
        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        VULKAN_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_info));
}

void end_commands(const VkQueue queue, const VkCommandBuffer command_buffer)
{
        VULKAN_CHECK(vkEndCommandBuffer(command_buffer));
        queue_submit(command_buffer, queue);
        VULKAN_CHECK(vkQueueWaitIdle(queue));
}

VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_sizes(
        const Device& device,
        const VkAccelerationStructureGeometryKHR& geometry,
        const VkAccelerationStructureTypeKHR type,
        const std::uint32_t primitive_count)
{
        VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info{};
        build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_geometry_info.type = type;
        build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
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
        const Device& device,
        const CommandPool& compute_command_pool,
        const Queue& compute_queue,
        const VkAccelerationStructureBuildSizesInfoKHR& build_sizes_info,
        const VkAccelerationStructureGeometryKHR& geometry,
        const VkAccelerationStructureTypeKHR type,
        const VkAccelerationStructureKHR acceleration_structure,
        const std::uint32_t primitive_count)
{
        const BufferWithMemory scratch_buffer(
                BufferMemoryType::DEVICE_LOCAL, device, {compute_queue.family_index()},
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                build_sizes_info.buildScratchSize);

        VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info{};
        build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_geometry_info.type = type;
        build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
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

        const handle::CommandBuffer command_buffer(device, compute_command_pool);

        begin_commands(command_buffer);

        vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &build_geometry_info, build_range_infos.data());

        end_commands(compute_queue, command_buffer);
}

void check_data(const std::span<const Vector3f>& vertices, const std::span<const std::uint32_t>& indices)
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
}

AccelerationStructure::AccelerationStructure(BufferWithMemory&& buffer, handle::AccelerationStructureKHR&& handle)
        : buffer_(std::move(buffer)),
          acceleration_structure_(std::move(handle)),
          device_address_(acceleration_structure_device_address(buffer_.buffer().device(), acceleration_structure_))
{
}

AccelerationStructure create_bottom_level_acceleration_structure(
        const Device& device,
        const CommandPool& compute_command_pool,
        const Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        const std::span<const Vector3f>& vertices,
        const std::span<const std::uint32_t>& indices,
        const std::optional<VkTransformMatrixKHR>& transform_matrix)
{
        check_data(vertices, indices);

        const std::uint32_t geometry_primitive_count = indices.size() / 3;

        const std::vector<std::uint32_t> buffer_family_indices = {compute_queue.family_index()};

        const BufferWithMemory vertex_buffer(
                BufferMemoryType::HOST_VISIBLE, device, buffer_family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                data_size(vertices));
        BufferMapper(vertex_buffer).write(vertices);

        const BufferWithMemory index_buffer(
                BufferMemoryType::HOST_VISIBLE, device, buffer_family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                data_size(indices));
        BufferMapper(index_buffer).write(indices);

        const std::optional<BufferWithMemory> transform_matrix_buffer = [&]
        {
                if (transform_matrix)
                {
                        std::optional<BufferWithMemory> buffer;
                        buffer.emplace(
                                BufferMemoryType::HOST_VISIBLE, device, buffer_family_indices,
                                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                                        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                                data_size(*transform_matrix));
                        BufferMapper(*buffer).write(*transform_matrix);
                        return buffer;
                }
                return std::optional<BufferWithMemory>();
        }();

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

        const VkAccelerationStructureBuildSizesInfoKHR build_sizes = acceleration_structure_build_sizes(
                device, geometry, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, geometry_primitive_count);

        BufferWithMemory acceleration_structure_buffer(
                BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                build_sizes.accelerationStructureSize);

        VkAccelerationStructureCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        create_info.buffer = acceleration_structure_buffer.buffer();
        create_info.size = build_sizes.accelerationStructureSize;
        create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        handle::AccelerationStructureKHR acceleration_structure(device, create_info);

        build_acceleration_structure(
                device, compute_command_pool, compute_queue, build_sizes, geometry,
                VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, acceleration_structure, geometry_primitive_count);

        return AccelerationStructure(std::move(acceleration_structure_buffer), std::move(acceleration_structure));
}

AccelerationStructure create_top_level_acceleration_structure(
        const Device& device,
        const CommandPool& compute_command_pool,
        const Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        const std::span<const std::uint64_t>& bottom_level_references)
{
        if (bottom_level_references.empty())
        {
                error("No bottom level acceleration structure references for top level acceleration structure");
        }

        const std::uint32_t geometry_primitive_count = bottom_level_references.size();

        static constexpr VkTransformMatrixKHR TRANSFORM = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}};

        std::vector<VkAccelerationStructureInstanceKHR> instances(bottom_level_references.size());
        for (std::size_t i = 0; i < bottom_level_references.size(); ++i)
        {
                instances[i] = {};
                instances[i].transform = TRANSFORM;
                instances[i].instanceCustomIndex = 0;
                instances[i].mask = 0xFF;
                instances[i].instanceShaderBindingTableRecordOffset = 0;
                instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
                instances[i].accelerationStructureReference = bottom_level_references[i];
        }

        const BufferWithMemory instance_buffer(
                BufferMemoryType::HOST_VISIBLE, device, {compute_queue.family_index()},
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                data_size(instances));
        BufferMapper(instance_buffer).write(instances);

        VkDeviceOrHostAddressConstKHR instance_buffer_device_address{};
        instance_buffer_device_address.deviceAddress = instance_buffer.device_address();

        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometry.geometry.instances.arrayOfPointers = VK_FALSE;
        geometry.geometry.instances.data = instance_buffer_device_address;

        const VkAccelerationStructureBuildSizesInfoKHR build_sizes = acceleration_structure_build_sizes(
                device, geometry, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, geometry_primitive_count);

        BufferWithMemory acceleration_structure_buffer(
                BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                build_sizes.accelerationStructureSize);

        VkAccelerationStructureCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        create_info.buffer = acceleration_structure_buffer.buffer();
        create_info.size = build_sizes.accelerationStructureSize;
        create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

        handle::AccelerationStructureKHR acceleration_structure(device, create_info);

        build_acceleration_structure(
                device, compute_command_pool, compute_queue, build_sizes, geometry,
                VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, acceleration_structure, geometry_primitive_count);

        return AccelerationStructure(std::move(acceleration_structure_buffer), std::move(acceleration_structure));
}
}
