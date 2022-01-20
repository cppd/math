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

#include "ray_tracing.h"

#include <src/com/container.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/error.h>
#include <src/vulkan/extensions.h>
#include <src/vulkan/queue.h>

namespace ns::gpu::renderer
{
namespace
{
VkDeviceAddress buffer_device_address(const VkDevice device, const VkBuffer buffer)
{
        VkBufferDeviceAddressInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = buffer;
        return vkGetBufferDeviceAddress(device, &info);
}

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

class AccelerationStructure final
{
        vulkan::BufferWithMemory buffer_;
        vulkan::handle::AccelerationStructureKHR acceleration_structure_;
        VkDeviceAddress device_address_;

public:
        AccelerationStructure(
                const vulkan::Device& device,
                vulkan::BufferWithMemory&& buffer,
                vulkan::handle::AccelerationStructureKHR&& handle)
                : buffer_(std::move(buffer)),
                  acceleration_structure_(std::move(handle)),
                  device_address_(acceleration_structure_device_address(device, acceleration_structure_))
        {
        }
};

AccelerationStructure create_bottom_level_acceleration_structure(
        const vulkan::Device& device,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const std::vector<std::uint32_t>& family_indices)
{
        const std::vector<Vector3f> vertices = {{Vector3f{0, 1, 0}}, {Vector3f{-1, 0, 0}}, {Vector3f{1, 0, 0}}};
        const std::vector<uint32_t> indices = {0, 1, 2};
        const VkTransformMatrixKHR transform_matrix = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}};

        vulkan::BufferWithMemory vertex_buffer(
                vulkan::BufferMemoryType::HOST_VISIBLE, device, family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                data_size(vertices));
        vulkan::BufferMapper(vertex_buffer).write(vertices);

        vulkan::BufferWithMemory index_buffer(
                vulkan::BufferMemoryType::HOST_VISIBLE, device, family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                data_size(indices));
        vulkan::BufferMapper(index_buffer).write(indices);

        vulkan::BufferWithMemory transform_matrix_buffer(
                vulkan::BufferMemoryType::HOST_VISIBLE, device, family_indices,
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                        | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                data_size(transform_matrix));
        vulkan::BufferMapper(transform_matrix_buffer).write(transform_matrix);

        VkDeviceOrHostAddressConstKHR vertex_buffer_device_address{};
        VkDeviceOrHostAddressConstKHR index_buffer_device_address{};
        VkDeviceOrHostAddressConstKHR transform_buffer_device_address{};

        vertex_buffer_device_address.deviceAddress = buffer_device_address(device, vertex_buffer.buffer());
        index_buffer_device_address.deviceAddress = buffer_device_address(device, index_buffer.buffer());
        transform_buffer_device_address.deviceAddress = buffer_device_address(device, transform_matrix_buffer.buffer());

        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometry.geometry.triangles.vertexData = vertex_buffer_device_address;
        geometry.geometry.triangles.maxVertex = vertices.size() - 1;
        geometry.geometry.triangles.vertexStride = sizeof(vertices[0]);
        geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        geometry.geometry.triangles.indexData = index_buffer_device_address;
        geometry.geometry.triangles.transformData = transform_buffer_device_address;

        VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info_sizes{};
        build_geometry_info_sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        build_geometry_info_sizes.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        build_geometry_info_sizes.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        build_geometry_info_sizes.geometryCount = 1;
        build_geometry_info_sizes.pGeometries = &geometry;

        const std::array<std::uint32_t, 1> geometry_primitive_count = {1};

        VkAccelerationStructureBuildSizesInfoKHR build_sizes_info{};
        build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
                device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_geometry_info_sizes,
                geometry_primitive_count.data(), &build_sizes_info);

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

        {
                vulkan::BufferWithMemory scratch_buffer(
                        vulkan::BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        build_sizes_info.buildScratchSize);

                VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info{};
                build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
                build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
                build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
                build_geometry_info.dstAccelerationStructure = acceleration_structure;
                build_geometry_info.geometryCount = 1;
                build_geometry_info.pGeometries = &geometry;
                build_geometry_info.scratchData.deviceAddress = buffer_device_address(device, scratch_buffer.buffer());

                VkAccelerationStructureBuildRangeInfoKHR build_range_info{};
                build_range_info.primitiveCount = geometry_primitive_count[0];
                build_range_info.primitiveOffset = 0;
                build_range_info.firstVertex = 0;
                build_range_info.transformOffset = 0;

                const std::vector<VkAccelerationStructureBuildRangeInfoKHR*> build_range_infos = {&build_range_info};

                vulkan::handle::CommandBuffer command_buffer(device, graphics_command_pool);

                begin_commands(command_buffer);

                vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &build_geometry_info, build_range_infos.data());

                end_commands(graphics_queue, command_buffer);
        }

        return AccelerationStructure(
                device, std::move(acceleration_structure_buffer), std::move(acceleration_structure));
}
}

void create_ray_tracing_data(
        const vulkan::Device* const device,
        const vulkan::CommandPool* const graphics_command_pool,
        const vulkan::Queue* const graphics_queue)
{
        create_bottom_level_acceleration_structure(
                *device, *graphics_command_pool, *graphics_queue, {graphics_command_pool->family_index()});
}
}
