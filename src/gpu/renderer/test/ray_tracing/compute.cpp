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

#include "compute.h"

#include "descriptors.h"
#include "program_ray_query.h"
#include "program_ray_tracing.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/group_count.h>
#include <src/image/file_save.h>
#include <src/image/image.h>
#include <src/settings/directory.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <string_view>

namespace ns::gpu::renderer::test
{
namespace
{
constexpr unsigned GROUP_SIZE = 16;

void save_to_file(const std::string_view name, const image::Image<2>& image)
{
        image::save(settings::test_directory() / path_from_utf8(name), image::ImageView<2>(image));
}

void run_ray_tracing_commands(
        const VkDevice device,
        const VkCommandPool compute_command_pool,
        const VkQueue queue,
        const RayTracingProgram& program,
        const RayTracingMemory& memory,
        const unsigned width,
        const unsigned height)
{
        const auto commands = [&](const VkCommandBuffer command_buffer)
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, program.pipeline());

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, program.pipeline_layout(),
                        memory.set_number(), 1 /*set count*/, &memory.descriptor_set(), 0, nullptr);

                program.command_trace_rays(command_buffer, width, height, 1);
        };

        vulkan::run_commands(device, compute_command_pool, queue, commands);
}

void run_ray_query_commands(
        const VkDevice device,
        const VkCommandPool compute_command_pool,
        const VkQueue queue,
        const RayQueryProgram& program,
        const RayTracingMemory& memory,
        const unsigned width,
        const unsigned height)
{
        const auto commands = [&](const VkCommandBuffer command_buffer)
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, program.pipeline());

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, program.pipeline_layout(), memory.set_number(),
                        1 /*set count*/, &memory.descriptor_set(), 0, nullptr);

                vkCmdDispatch(command_buffer, group_count(width, GROUP_SIZE), group_count(height, GROUP_SIZE), 1);
        };

        vulkan::run_commands(device, compute_command_pool, queue, commands);
}
}

image::Image<2> ray_tracing(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const RayTracingImage& ray_tracing_image,
        const VkAccelerationStructureKHR acceleration_structure,
        const std::string_view& file_name)
{
        ASSERT(compute_command_pool.family_index() == compute_queue.family_index());

        const RayTracingProgram program(device, {compute_command_pool.family_index()});

        const RayTracingMemory memory(
                device.handle(), program.descriptor_set_layout(), program.descriptor_set_layout_bindings());

        memory.set_acceleration_structure(acceleration_structure);
        memory.set_image(ray_tracing_image.image_view());

        run_ray_tracing_commands(
                device.handle(), compute_command_pool.handle(), compute_queue.handle(), program, memory,
                ray_tracing_image.width(), ray_tracing_image.height());

        image::Image<2> image = ray_tracing_image.image();
        save_to_file(file_name, image);
        return image;
}

image::Image<2> ray_query(
        const VkDevice device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const RayTracingImage& ray_tracing_image,
        const VkAccelerationStructureKHR acceleration_structure,
        const std::string_view& file_name)
{
        ASSERT(compute_command_pool.family_index() == compute_queue.family_index());

        const RayQueryProgram program(device, GROUP_SIZE);

        const RayTracingMemory memory(
                device, program.descriptor_set_layout(), program.descriptor_set_layout_bindings());

        memory.set_acceleration_structure(acceleration_structure);
        memory.set_image(ray_tracing_image.image_view());

        run_ray_query_commands(
                device, compute_command_pool.handle(), compute_queue.handle(), program, memory,
                ray_tracing_image.width(), ray_tracing_image.height());

        image::Image<2> image = ray_tracing_image.image();
        save_to_file(file_name, image);
        return image;
}
}
