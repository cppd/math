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

#include <src/com/container.h>
#include <src/com/print.h>
#include <src/vulkan/error.h>
#include <src/vulkan/extensions.h>
#include <src/vulkan/queue.h>

namespace ns::gpu::renderer
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
        vulkan::queue_submit(command_buffer, queue);
        VULKAN_CHECK(vkQueueWaitIdle(queue));
}

void build_acceleration_structure(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const VkAccelerationStructureBuildSizesInfoKHR& build_sizes_info,
        const VkAccelerationStructureGeometryKHR& geometry,
        const VkAccelerationStructureTypeKHR type,
        const VkAccelerationStructureKHR acceleration_structure,
        const std::uint32_t primitive_count)
{
        vulkan::BufferWithMemory scratch_buffer(
                vulkan::BufferMemoryType::DEVICE_LOCAL, device, {compute_queue.family_index()},
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

        vulkan::handle::CommandBuffer command_buffer(device, compute_command_pool);

        begin_commands(command_buffer);

        vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &build_geometry_info, build_range_infos.data());

        end_commands(compute_queue, command_buffer);
}
}

AccelerationStructure::AccelerationStructure(
        const vulkan::Device& device,
        vulkan::BufferWithMemory&& buffer,
        vulkan::handle::AccelerationStructureKHR&& handle)
        : buffer_(std::move(buffer)),
          acceleration_structure_(std::move(handle)),
          device_address_(acceleration_structure_device_address(device, acceleration_structure_))
{
}

AccelerationStructure create_bottom_level_acceleration_structure(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        const std::span<const Vector3f>& vertices,
        const std::span<const std::uint32_t>& indices,
        const std::optional<VkTransformMatrixKHR>& transform_matrix)
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

        const std::uint32_t geometry_primitive_count = indices.size() / 3;

        vulkan::BufferWithMemory vertex_buffer(
                vulkan::BufferMemoryType::HOST_VISIBLE, device, {compute_queue.family_index()},
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                data_size(vertices));

        vulkan::BufferMapper(vertex_buffer).write(vertices);

        vulkan::BufferWithMemory index_buffer(
                vulkan::BufferMemoryType::HOST_VISIBLE, device, {compute_queue.family_index()},
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                data_size(indices));

        vulkan::BufferMapper(index_buffer).write(indices);

        std::optional<vulkan::BufferWithMemory> transform_matrix_buffer;
        if (transform_matrix)
        {
                transform_matrix_buffer.emplace(
                        vulkan::BufferMemoryType::HOST_VISIBLE, device, std::vector({compute_queue.family_index()}),
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                                | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                        data_size(*transform_matrix));

                vulkan::BufferMapper(*transform_matrix_buffer).write(*transform_matrix);
        }

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

        VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info_sizes{};
        build_geometry_info_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_geometry_info_sizes.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        build_geometry_info_sizes.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        build_geometry_info_sizes.geometryCount = 1;
        build_geometry_info_sizes.pGeometries = &geometry;

        VkAccelerationStructureBuildSizesInfoKHR build_sizes_info{};
        build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
                device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_geometry_info_sizes,
                &geometry_primitive_count, &build_sizes_info);

        vulkan::BufferWithMemory acceleration_structure_buffer(
                vulkan::BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                build_sizes_info.accelerationStructureSize);

        VkAccelerationStructureCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        create_info.buffer = acceleration_structure_buffer.buffer();
        create_info.size = build_sizes_info.accelerationStructureSize;
        create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        vulkan::handle::AccelerationStructureKHR acceleration_structure(device, create_info);

        build_acceleration_structure(
                device, compute_command_pool, compute_queue, build_sizes_info, geometry,
                VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, acceleration_structure, geometry_primitive_count);

        return AccelerationStructure(
                device, std::move(acceleration_structure_buffer), std::move(acceleration_structure));
}

AccelerationStructure create_top_level_acceleration_structure(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        const AccelerationStructure& bottom_level_acceleration_structure)
{
        const VkTransformMatrixKHR transform_matrix = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}};

        VkAccelerationStructureInstanceKHR instance{};
        instance.transform = transform_matrix;
        instance.instanceCustomIndex = 0;
        instance.mask = 0xff;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = bottom_level_acceleration_structure.device_address();

        vulkan::BufferWithMemory instance_buffer(
                vulkan::BufferMemoryType::HOST_VISIBLE, device, {compute_queue.family_index()},
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                data_size(instance));
        vulkan::BufferMapper(instance_buffer).write(instance);

        VkDeviceOrHostAddressConstKHR instance_buffer_device_address{};
        instance_buffer_device_address.deviceAddress = instance_buffer.device_address();

        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometry.geometry.instances.arrayOfPointers = VK_FALSE;
        geometry.geometry.instances.data = instance_buffer_device_address;

        VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info_sizes{};
        build_geometry_info_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_geometry_info_sizes.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        build_geometry_info_sizes.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        build_geometry_info_sizes.geometryCount = 1;
        build_geometry_info_sizes.pGeometries = &geometry;

        const std::uint32_t geometry_primitive_count = 1;

        VkAccelerationStructureBuildSizesInfoKHR build_sizes_info{};
        build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
                device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_geometry_info_sizes,
                &geometry_primitive_count, &build_sizes_info);

        vulkan::BufferWithMemory acceleration_structure_buffer(
                vulkan::BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                build_sizes_info.accelerationStructureSize);

        VkAccelerationStructureCreateInfoKHR create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        create_info.buffer = acceleration_structure_buffer.buffer();
        create_info.size = build_sizes_info.accelerationStructureSize;
        create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

        vulkan::handle::AccelerationStructureKHR acceleration_structure(device, create_info);

        build_acceleration_structure(
                device, compute_command_pool, compute_queue, build_sizes_info, geometry,
                VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, acceleration_structure, geometry_primitive_count);

        return AccelerationStructure(
                device, std::move(acceleration_structure_buffer), std::move(acceleration_structure));
}
}
