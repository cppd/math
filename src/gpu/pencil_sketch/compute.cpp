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

#include "compute.h"

#include "shaders/compute.h"

#include <src/com/error.h>
#include <src/com/group_count.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>

#include <vulkan/vulkan_core.h>

#include <memory>
#include <thread>

namespace ns::gpu::pencil_sketch
{
namespace
{
constexpr unsigned GROUP_SIZE = 16;

void image_barrier_before(const VkCommandBuffer command_buffer, const VkImage image)
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

void image_barrier_after(const VkCommandBuffer command_buffer, const VkImage image)
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
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const vulkan::Device* const device_;

        ComputeProgram program_;
        ComputeMemory memory_;

        unsigned groups_x_ = 0;
        unsigned groups_y_ = 0;

        VkImage image_ = VK_NULL_HANDLE;

        void compute_commands(const VkCommandBuffer command_buffer) const override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                ASSERT(groups_x_ > 0 && groups_y_ > 0);

                image_barrier_before(command_buffer, image_);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, program_.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, program_.pipeline_layout(),
                        ComputeMemory::set_number(), 1, &memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, groups_x_, groups_y_, 1);

                image_barrier_after(command_buffer, image_);
        }

        void create_buffers(
                const VkSampler sampler,
                const vulkan::ImageWithMemory& input,
                const vulkan::ImageWithMemory& objects,
                const numerical::Region<2, int>& rectangle,
                const vulkan::ImageWithMemory& output) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                image_ = output.image().handle();

                //

                ASSERT(sampler != VK_NULL_HANDLE);

                ASSERT(input.image().type() == VK_IMAGE_TYPE_2D);
                ASSERT(objects.image().type() == VK_IMAGE_TYPE_2D);
                ASSERT(output.image().type() == VK_IMAGE_TYPE_2D);

                ASSERT(input.image().extent().width == objects.image().extent().width);
                ASSERT(input.image().extent().height == objects.image().extent().height);

                ASSERT(rectangle.is_positive());
                ASSERT(rectangle.width() == static_cast<int>(output.image().extent().width));
                ASSERT(rectangle.height() == static_cast<int>(output.image().extent().height));
                ASSERT(rectangle.x1() <= static_cast<int>(objects.image().extent().width));
                ASSERT(rectangle.y1() <= static_cast<int>(objects.image().extent().height));

                memory_.set_input(sampler, input.image_view());
                memory_.set_object_image(objects.image_view());
                memory_.set_output_image(output.image_view());

                //

                program_.create_pipeline(GROUP_SIZE, rectangle);

                groups_x_ = group_count<unsigned>(rectangle.width(), GROUP_SIZE);
                groups_y_ = group_count<unsigned>(rectangle.height(), GROUP_SIZE);
        }

        void delete_buffers() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                groups_x_ = 0;
                groups_y_ = 0;

                program_.delete_pipeline();

                image_ = VK_NULL_HANDLE;
        }

public:
        explicit Impl(const vulkan::Device* const device)
                : device_(device),
                  program_(device_->handle()),
                  memory_(device_->handle(), program_.descriptor_set_layout())
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                device_->wait_idle_noexcept("pencil sketch compute destructor");
        }
};
}

std::unique_ptr<Compute> create_compute(const vulkan::Device* const device)
{
        return std::make_unique<Impl>(device);
}
}
