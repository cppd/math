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

#include "acceleration_structure.h"
#include "descriptors.h"
#include "image.h"
#include "program.h"

#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>

namespace ns::gpu::renderer
{
namespace
{
vulkan::handle::CommandBuffer create_command_buffer(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const RayTracingProgram& program,
        const RayTracingMemory& memory,
        unsigned width,
        unsigned height)
{
        vulkan::handle::CommandBuffer command_buffer(device, compute_command_pool);

        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VULKAN_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_info));

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, program.pipeline());

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, program.pipeline_layout(), memory.set_number(),
                1 /*set count*/, &memory.descriptor_set(), 0, nullptr);

        program.command_trace_rays(command_buffer, width, height, 1);

        VULKAN_CHECK(vkEndCommandBuffer(command_buffer));

        return command_buffer;
}

std::vector<AccelerationStructure> create_bottom_level(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue)
{
        std::vector<AccelerationStructure> bottom_level;

        constexpr std::array VERTICES_0 = std::to_array<Vector3f>({{-0.5, 1, 0}, {-1, 0, 0}, {0, 0, 0}, {-0.5, -1, 0}});
        constexpr std::array INDICES_0 = std::to_array<std::uint32_t>({0, 1, 2, 1, 2, 3});

        bottom_level.push_back(create_bottom_level_acceleration_structure(
                device, compute_command_pool, compute_queue, {compute_command_pool.family_index()}, VERTICES_0,
                INDICES_0, std::nullopt));

        constexpr std::array VERTICES_1 = std::to_array<Vector3f>({{0.5, 1, 0}, {1, 0, 0}, {0, 0, 0}, {0.5, -1, 0}});
        constexpr std::array INDICES_1 = std::to_array<std::uint32_t>({0, 1, 2, 1, 2, 3});

        bottom_level.push_back(create_bottom_level_acceleration_structure(
                device, compute_command_pool, compute_queue, {compute_command_pool.family_index()}, VERTICES_1,
                INDICES_1, std::nullopt));

        return bottom_level;
}

AccelerationStructure create_top_level(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<AccelerationStructure>& bottom_level)
{
        std::vector<std::uint64_t> references;
        references.reserve(bottom_level.size());
        for (const AccelerationStructure& v : bottom_level)
        {
                references.push_back(v.device_address());
        }
        return create_top_level_acceleration_structure(
                device, compute_command_pool, compute_queue, {compute_command_pool.family_index()}, references);
}
}

void create_ray_tracing_data(
        const vulkan::Device* const device,
        const vulkan::CommandPool* const compute_command_pool,
        const vulkan::Queue* const compute_queue)
{
        const RayTracingImage image(1000, 1000, device, compute_command_pool, compute_queue);

        const std::vector<AccelerationStructure> bottom_level =
                create_bottom_level(*device, *compute_command_pool, *compute_queue);

        const AccelerationStructure top_level =
                create_top_level(*device, *compute_command_pool, *compute_queue, bottom_level);

        const RayTracingProgram program(*device, {compute_command_pool->family_index()});

        const RayTracingMemory memory(
                *device, program.descriptor_set_layout(), program.descriptor_set_layout_bindings());

        memory.set_acceleration_structure(top_level.handle());
        memory.set_image(image.image_view());

        const vulkan::handle::CommandBuffer command_buffer =
                create_command_buffer(*device, *compute_command_pool, program, memory, image.width(), image.height());

        vulkan::queue_submit(command_buffer, *compute_queue);
        VULKAN_CHECK(vkQueueWaitIdle(*compute_queue));

        image.save_to_file("ray_tracing");
}
}
