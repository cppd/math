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

#include "descriptors.h"
#include "image.h"
#include "program_ray_query.h"
#include "program_ray_tracing.h"

#include "../../../com/groups.h"
#include "../../functionality.h"

#include <src/com/log.h>
#include <src/test/test.h>
#include <src/vulkan/acceleration_structure.h>
#include <src/vulkan/create.h>
#include <src/vulkan/device_compute.h>
#include <src/vulkan/error.h>
#include <src/vulkan/instance.h>
#include <src/vulkan/queue.h>

namespace ns::gpu::renderer::test
{
namespace
{
constexpr unsigned GROUP_SIZE = 16;

vulkan::handle::CommandBuffer create_ray_tracing_command_buffer(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const RayTracingProgram& program,
        const RayTracingMemory& memory,
        const unsigned width,
        const unsigned height)
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

vulkan::handle::CommandBuffer create_ray_query_command_buffer(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const RayQueryProgram& program,
        const RayTracingMemory& memory,
        const unsigned width,
        const unsigned height)
{
        vulkan::handle::CommandBuffer command_buffer(device, compute_command_pool);

        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VULKAN_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_info));

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, program.pipeline());

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, program.pipeline_layout(), memory.set_number(),
                1 /*set count*/, &memory.descriptor_set(), 0, nullptr);

        vkCmdDispatch(command_buffer, group_count(width, GROUP_SIZE), group_count(height, GROUP_SIZE), 1);

        VULKAN_CHECK(vkEndCommandBuffer(command_buffer));

        return command_buffer;
}

std::vector<vulkan::BottomLevelAccelerationStructure> create_bottom_level(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices)
{
        constexpr std::array VERTICES_0 = std::to_array<Vector3f>({{-0.5, 1, 0}, {-1, 0, 0}, {0, 0, 0}, {-0.5, -1, 0}});
        constexpr std::array INDICES_0 = std::to_array<std::uint32_t>({0, 1, 2, 1, 2, 3});

        constexpr std::array VERTICES_1 = std::to_array<Vector3f>({{0.5, 1, 0}, {1, 0, 0}, {0, 0, 0}, {0.5, -1, 0}});
        constexpr std::array INDICES_1 = std::to_array<std::uint32_t>({0, 1, 2, 1, 2, 3});

        std::vector<vulkan::BottomLevelAccelerationStructure> bottom_level;

        bottom_level.push_back(create_bottom_level_acceleration_structure(
                device, compute_command_pool, compute_queue, family_indices, VERTICES_0, INDICES_0, std::nullopt));

        bottom_level.push_back(create_bottom_level_acceleration_structure(
                device, compute_command_pool, compute_queue, family_indices, VERTICES_1, INDICES_1, std::nullopt));

        return bottom_level;
}

std::vector<VkTransformMatrixKHR> create_matrices()
{
        constexpr VkTransformMatrixKHR MATRIX_0{{{1, 0, 0, 0.1}, {0, 1, 0, 0}, {0, 0, 1, 0}}};
        constexpr VkTransformMatrixKHR MATRIX_1{{{1, 0, 0, -0.1}, {0, 1, 0, 0}, {0, 0, 1, 0}}};

        std::vector<VkTransformMatrixKHR> matrices;
        matrices.push_back(MATRIX_0);
        matrices.push_back(MATRIX_1);
        return matrices;
}

vulkan::TopLevelAccelerationStructure create_top_level(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        const std::vector<vulkan::BottomLevelAccelerationStructure>& bottom_level,
        const std::vector<VkTransformMatrixKHR>& matrices)
{
        std::vector<std::uint64_t> references;
        references.reserve(bottom_level.size());
        for (const vulkan::BottomLevelAccelerationStructure& v : bottom_level)
        {
                references.push_back(v.device_address());
        }

        return create_top_level_acceleration_structure(
                device, compute_command_pool, compute_queue, family_indices, references, matrices);
}

void ray_tracing(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const RayTracingImage& image,
        const VkAccelerationStructureKHR acceleration_structure,
        const std::string_view& file_name)
{
        const RayTracingProgram program(device, {compute_command_pool.family_index()});

        const RayTracingMemory memory(
                device, program.descriptor_set_layout(), program.descriptor_set_layout_bindings());

        memory.set_acceleration_structure(acceleration_structure);
        memory.set_image(image.image_view());

        const vulkan::handle::CommandBuffer command_buffer = create_ray_tracing_command_buffer(
                device, compute_command_pool, program, memory, image.width(), image.height());

        vulkan::queue_submit(command_buffer, compute_queue);
        VULKAN_CHECK(vkQueueWaitIdle(compute_queue));

        image.save_to_file(file_name);
}

void ray_query(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const RayTracingImage& image,
        const VkAccelerationStructureKHR acceleration_structure,
        const std::string_view& file_name)
{
        const RayQueryProgram program(device, GROUP_SIZE);

        const RayTracingMemory memory(
                device, program.descriptor_set_layout(), program.descriptor_set_layout_bindings());

        memory.set_acceleration_structure(acceleration_structure);
        memory.set_image(image.image_view());

        const vulkan::handle::CommandBuffer command_buffer = create_ray_query_command_buffer(
                device, compute_command_pool, program, memory, image.width(), image.height());

        vulkan::queue_submit(command_buffer, compute_queue);
        VULKAN_CHECK(vkQueueWaitIdle(compute_queue));

        image.save_to_file(file_name);
}

void test_ray_tracing()
{
        const vulkan::DeviceCompute device_compute(
                vulkan::PhysicalDeviceSearchType::RANDOM, vulkan::Instance::handle(),
                device_ray_tracing_functionality());

        const vulkan::Device& device = device_compute.device();

        if (!ray_tracing_supported(device))
        {
                return;
        }

        const vulkan::Queue& queue = device_compute.compute_queue();

        const vulkan::CommandPool command_pool =
                vulkan::create_command_pool(device, device_compute.compute_family_index());

        const RayTracingImage image(1000, 1000, device, &command_pool, &queue);

        const std::vector<std::uint32_t> family_indices{command_pool.family_index()};

        const std::vector<vulkan::BottomLevelAccelerationStructure> bottom_level =
                create_bottom_level(device, command_pool, queue, family_indices);

        std::vector<VkTransformMatrixKHR> matrices = create_matrices();

        const vulkan::TopLevelAccelerationStructure top_level =
                create_top_level(device, command_pool, queue, family_indices, bottom_level, matrices);

        ray_tracing(device, command_pool, queue, image, top_level.handle(), "ray_tracing");
        ray_query(device, command_pool, queue, image, top_level.handle(), "ray_query");

        for (VkTransformMatrixKHR& m : matrices)
        {
                m.matrix[0][3] += 0.1;
        }
        top_level.update_matrices(device, command_pool, queue, matrices);

        ray_tracing(device, command_pool, queue, image, top_level.handle(), "ray_tracing_update");
        ray_query(device, command_pool, queue, image, top_level.handle(), "ray_query_update");
}

void test()
{
        LOG("Test Vulkan ray tracing");
        test_ray_tracing();
        LOG("Test Vulkan ray tracing passed");
}
}

TEST_SMALL("Vulkan ray tracing", test)
}
