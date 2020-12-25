/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "../com/groups.h"
#include "shaders/compute.h"

#include <src/com/error.h>

#include <optional>
#include <thread>

namespace ns::gpu::pencil_sketch
{
namespace
{
// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
};
// clang-format on

constexpr unsigned GROUP_SIZE = 16;

void image_barrier_before(VkCommandBuffer command_buffer, VkImage image)
{
        ASSERT(command_buffer != VK_NULL_HANDLE && image != VK_NULL_HANDLE);

        VkImageMemoryBarrier barrier = {};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);
}

void image_barrier_after(VkCommandBuffer command_buffer, VkImage image)
{
        ASSERT(command_buffer != VK_NULL_HANDLE && image != VK_NULL_HANDLE);

        VkImageMemoryBarrier barrier = {};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);
}

class Impl final : public Compute
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const vulkan::VulkanInstance& m_instance;

        ComputeProgram m_program;
        ComputeMemory m_memory;

        unsigned m_groups_x = 0;
        unsigned m_groups_y = 0;

        VkImage m_image = VK_NULL_HANDLE;

        void compute_commands(VkCommandBuffer command_buffer) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                ASSERT(m_groups_x > 0 && m_groups_y > 0);

                image_barrier_before(command_buffer, m_image);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_program.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_program.pipeline_layout(),
                        ComputeMemory::set_number(), 1, &m_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_groups_x, m_groups_y, 1);

                image_barrier_after(command_buffer, m_image);
        }

        void create_buffers(
                VkSampler sampler,
                const vulkan::ImageWithMemory& input,
                const vulkan::ImageWithMemory& objects,
                const Region<2, int>& rectangle,
                const vulkan::ImageWithMemory& output) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_image = output.image();

                //

                ASSERT(sampler != VK_NULL_HANDLE);
                ASSERT(input.width() == objects.width() && input.height() == objects.height());

                ASSERT(rectangle.is_positive());
                ASSERT(rectangle.width() == static_cast<int>(output.width()));
                ASSERT(rectangle.height() == static_cast<int>(output.height()));
                ASSERT(rectangle.x1() <= static_cast<int>(objects.width()));
                ASSERT(rectangle.y1() <= static_cast<int>(objects.height()));

                m_memory.set_input(sampler, input);
                m_memory.set_object_image(objects);
                m_memory.set_output_image(output);

                //

                m_program.create_pipeline(GROUP_SIZE, rectangle);

                m_groups_x = group_count(rectangle.width(), GROUP_SIZE);
                m_groups_y = group_count(rectangle.height(), GROUP_SIZE);
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_groups_x = 0;
                m_groups_y = 0;

                m_program.delete_pipeline();

                m_image = VK_NULL_HANDLE;
        }

public:
        explicit Impl(const vulkan::VulkanInstance& instance)
                : m_instance(instance),
                  m_program(instance.device()),
                  m_memory(instance.device(), m_program.descriptor_set_layout())
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_instance.device_wait_idle_noexcept("the Vulkan pencil sketch compute destructor");
        }
};
}

std::vector<vulkan::PhysicalDeviceFeatures> Compute::required_device_features()
{
        return REQUIRED_DEVICE_FEATURES;
}

std::unique_ptr<Compute> create_compute(const vulkan::VulkanInstance& instance)
{
        return std::make_unique<Impl>(instance);
}
}
