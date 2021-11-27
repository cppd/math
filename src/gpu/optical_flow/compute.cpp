/*
Copyright (C) 2017-2021 Topological Manifold

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

/*
Aaftab Munshi, Benedict R. Gaster, Timothy G. Mattson, James Fung, Dan Ginsburg.
OpenCL Programming Guide.
Addison-Wesley, 2011.
19. Optical Flow

Salil Kapur, Nisarg Thakkar.
Mastering OpenCV Android Application Programming.
Packt Publishing, 2015.
5. Tracking Objects in Videos
*/

#include "compute.h"

#include "create.h"
#include "function.h"
#include "option.h"

#include "shaders/downsample.h"
#include "shaders/flow.h"
#include "shaders/grayscale.h"
#include "shaders/sobel.h"

#include <src/numerical/vec.h>
#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>

#include <array>
#include <thread>
#include <vector>

namespace ns::gpu::optical_flow
{
namespace
{
constexpr VkFormat IMAGE_FORMAT = VK_FORMAT_R32_SFLOAT;

vulkan::DeviceFeatures device_features()
{
        return {};
}

void image_barrier(
        VkCommandBuffer command_buffer,
        const std::vector<VkImage>& images,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        VkAccessFlags src_access_mask,
        VkAccessFlags dst_access_mask)
{
        ASSERT(!images.empty());
        ASSERT(command_buffer != VK_NULL_HANDLE);
        ASSERT(std::all_of(
                images.cbegin(), images.cend(),
                [](VkImage image)
                {
                        return image != VK_NULL_HANDLE;
                }));

        std::vector<VkImageMemoryBarrier> barriers(images.size());

        for (std::size_t i = 0; i < images.size(); ++i)
        {
                barriers[i] = {};

                barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

                barriers[i].oldLayout = old_layout;
                barriers[i].newLayout = new_layout;

                barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                barriers[i].image = images[i];

                barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barriers[i].subresourceRange.baseMipLevel = 0;
                barriers[i].subresourceRange.levelCount = 1;
                barriers[i].subresourceRange.baseArrayLayer = 0;
                barriers[i].subresourceRange.layerCount = 1;

                barriers[i].srcAccessMask = src_access_mask;
                barriers[i].dstAccessMask = dst_access_mask;
        }

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, barriers.size(), barriers.data());
}

void image_barrier(
        VkCommandBuffer command_buffer,
        VkImage image,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        VkAccessFlags src_access_mask,
        VkAccessFlags dst_access_mask)
{
        image_barrier(command_buffer, std::vector{image}, old_layout, new_layout, src_access_mask, dst_access_mask);
}

void buffer_barrier(VkCommandBuffer command_buffer, VkBuffer buffer, VkPipelineStageFlags dst_stage_mask)
{
        ASSERT(buffer != VK_NULL_HANDLE);

        VkBufferMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = buffer;
        barrier.offset = 0;
        barrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, dst_stage_mask, VK_DEPENDENCY_BY_REGION_BIT, 0,
                nullptr, 1, &barrier, 0, nullptr);
}

std::vector<const vulkan::Buffer*> to_buffer_pointers(const std::vector<vulkan::BufferWithMemory>& buffers)
{
        std::vector<const vulkan::Buffer*> result;
        result.reserve(buffers.size());
        for (const vulkan::BufferWithMemory& buffer : buffers)
        {
                result.push_back(&buffer.buffer());
        }
        return result;
}

void begin_command_buffer(const VkCommandBuffer command_buffer)
{
        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VULKAN_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_info));
}

void end_command_buffer(const VkCommandBuffer command_buffer)
{
        VULKAN_CHECK(vkEndCommandBuffer(command_buffer));
}

class Impl final : public Compute
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const vulkan::VulkanInstance* const instance_;
        const vulkan::Device* const device_;

        const vulkan::CommandPool* const compute_command_pool_;
        const vulkan::Queue* const compute_queue_;

        vulkan::handle::Semaphore semaphore_first_pyramid_;
        vulkan::handle::Semaphore semaphore_;

        std::optional<vulkan::handle::CommandBuffer> command_buffer_first_pyramid_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_;

        std::array<std::vector<vulkan::ImageWithMemory>, 2> images_;
        std::vector<vulkan::ImageWithMemory> dx_;
        std::vector<vulkan::ImageWithMemory> dy_;
        std::vector<vulkan::BufferWithMemory> flow_buffers_;

        GrayscaleProgram grayscale_program_;
        GrayscaleMemory grayscale_memory_;
        Vector2i grayscale_groups_;

        DownsampleProgram downsample_program_;
        std::vector<DownsampleMemory> downsample_memory_;
        std::vector<Vector2i> downsample_groups_;

        SobelProgram sobel_program_;
        std::vector<SobelMemory> sobel_memory_;
        std::vector<Vector2i> sobel_groups_;

        FlowProgram flow_program_;
        std::vector<FlowMemory> flow_memory_;
        std::vector<Vector2i> flow_groups_;

        int i_index_ = -1;

        void commands_compute_image_pyramid(int index, VkCommandBuffer command_buffer)
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(downsample_memory_.size() == downsample_groups_.size());
                ASSERT(downsample_memory_.size() + 1 == images_[index].size());

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, grayscale_program_.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, grayscale_program_.pipeline_layout(),
                        GrayscaleMemory::set_number(), 1, &grayscale_memory_.descriptor_set(index), 0, nullptr);
                vkCmdDispatch(command_buffer, grayscale_groups_[0], grayscale_groups_[1], 1);

                image_barrier(
                        command_buffer, images_[index][0].image(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

                for (unsigned i = 0; i < downsample_groups_.size(); ++i)
                {
                        vkCmdBindPipeline(
                                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, downsample_program_.pipeline());
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, downsample_program_.pipeline_layout(),
                                DownsampleMemory::set_number(), 1, &downsample_memory_[i].descriptor_set(index), 0,
                                nullptr);
                        vkCmdDispatch(command_buffer, downsample_groups_[i][0], downsample_groups_[i][1], 1);

                        image_barrier(
                                command_buffer, images_[index][i + 1].image(), VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
                }
        }

        void commands_compute_dxdy(int index, VkCommandBuffer command_buffer) const
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(sobel_memory_.size() == sobel_groups_.size());
                ASSERT(sobel_groups_.size() == dx_.size());
                ASSERT(sobel_groups_.size() == dy_.size());

                for (unsigned i = 0; i < sobel_groups_.size(); ++i)
                {
                        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, sobel_program_.pipeline());
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, sobel_program_.pipeline_layout(),
                                SobelMemory::set_number(), 1, &sobel_memory_[i].descriptor_set(index), 0, nullptr);
                        vkCmdDispatch(command_buffer, sobel_groups_[i][0], sobel_groups_[i][1], 1);
                }

                std::vector<VkImage> images;
                images.reserve(dx_.size() + dy_.size());
                for (unsigned i = 0; i < sobel_groups_.size(); ++i)
                {
                        images.push_back(dx_[i].image());
                        images.push_back(dy_[i].image());
                }
                image_barrier(
                        command_buffer, images, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
                        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
        }

        void commands_compute_optical_flow(int index, VkCommandBuffer command_buffer, VkBuffer top_flow) const
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(flow_memory_.size() == flow_groups_.size());
                ASSERT(flow_buffers_.size() + 1 == flow_groups_.size());

                for (int i = static_cast<int>(flow_groups_.size()) - 1; i >= 0; --i)
                {
                        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, flow_program_.pipeline());
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, flow_program_.pipeline_layout(),
                                FlowMemory::set_number(), 1, &flow_memory_[i].descriptor_set(index), 0, nullptr);
                        vkCmdDispatch(command_buffer, flow_groups_[i][0], flow_groups_[i][1], 1);

                        buffer_barrier(
                                command_buffer, (i != 0) ? flow_buffers_[i - 1].buffer() : top_flow,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
                }
        }

        void commands_images_to_sampler_layout(int index, VkCommandBuffer command_buffer)
        {
                for (const vulkan::ImageWithMemory& image : images_[index])
                {
                        image_barrier(
                                command_buffer, image.image(), VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, VK_ACCESS_SHADER_READ_BIT);
                }
        }

        void commands_images_to_general_layout(int index, VkCommandBuffer command_buffer)
        {
                for (const vulkan::ImageWithMemory& image : images_[index])
                {
                        image_barrier(
                                command_buffer, image.image(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_IMAGE_LAYOUT_GENERAL, 0, VK_ACCESS_SHADER_READ_BIT);
                }
        }

        void create_command_buffer_first_pyramid()
        {
                command_buffer_first_pyramid_ = vulkan::handle::CommandBuffer(*device_, *compute_command_pool_);

                const VkCommandBuffer command_buffer = *command_buffer_first_pyramid_;

                begin_command_buffer(command_buffer);

                commands_compute_image_pyramid(0, command_buffer);

                end_command_buffer(command_buffer);
        }

        void create_command_buffers(VkBuffer top_flow)
        {
                command_buffers_ = vulkan::handle::CommandBuffers(*device_, *compute_command_pool_, 2);

                for (int index = 0; index < 2; ++index)
                {
                        const VkCommandBuffer command_buffer = (*command_buffers_)[index];

                        begin_command_buffer(command_buffer);

                        // i — previous image, 1-i — current image
                        commands_compute_image_pyramid(1 - index, command_buffer);
                        commands_compute_dxdy(index, command_buffer);

                        commands_images_to_sampler_layout(1 - index, command_buffer);
                        commands_compute_optical_flow(index, command_buffer, top_flow);
                        commands_images_to_general_layout(1 - index, command_buffer);

                        end_command_buffer(command_buffer);
                }
        }

        VkSemaphore compute(const vulkan::Queue& queue, VkSemaphore wait_semaphore) override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                ASSERT(queue.family_index() == compute_command_pool_->family_index());
                ASSERT(command_buffers_ && command_buffers_->count() == 2);
                ASSERT(i_index_ == -1 || i_index_ == 0 || i_index_ == 1);

                if (i_index_ < 0)
                {
                        i_index_ = 0;

                        ASSERT(command_buffer_first_pyramid_);
                        vulkan::queue_submit(
                                wait_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, *command_buffer_first_pyramid_,
                                semaphore_first_pyramid_, queue);
                        wait_semaphore = semaphore_first_pyramid_;
                }
                else
                {
                        i_index_ = 1 - i_index_;
                }

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (*command_buffers_)[i_index_], semaphore_,
                        queue);

                return semaphore_;
        }

        void create_buffers(
                VkSampler sampler,
                const vulkan::ImageWithMemory& input,
                const Region<2, int>& rectangle,
                unsigned top_point_count_x,
                unsigned top_point_count_y,
                const vulkan::Buffer& top_points,
                const vulkan::Buffer& top_flow) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                ASSERT(sampler != VK_NULL_HANDLE);

                ASSERT(input.image().type() == VK_IMAGE_TYPE_2D);

                ASSERT(rectangle.is_positive());
                ASSERT(rectangle.x1() <= static_cast<int>(input.image().extent().width));
                ASSERT(rectangle.y1() <= static_cast<int>(input.image().extent().height));

                const std::vector<Vector2i> sizes = pyramid_sizes(
                        input.image().extent().width, input.image().extent().height, BOTTOM_IMAGE_MINIMUM_SIZE);

                const std::uint32_t family_index = compute_command_pool_->family_index();

                images_[0] = create_images(
                        *device_, *compute_command_pool_, *compute_queue_, sizes, IMAGE_FORMAT, family_index,
                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

                images_[1] = create_images(
                        *device_, *compute_command_pool_, *compute_queue_, sizes, IMAGE_FORMAT, family_index,
                        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

                dx_ = create_images(
                        *device_, *compute_command_pool_, *compute_queue_, sizes, IMAGE_FORMAT, family_index,
                        VK_IMAGE_USAGE_STORAGE_BIT);

                dy_ = create_images(
                        *device_, *compute_command_pool_, *compute_queue_, sizes, IMAGE_FORMAT, family_index,
                        VK_IMAGE_USAGE_STORAGE_BIT);

                flow_buffers_ = create_flow_buffers(*device_, sizes, family_index);

                constexpr Vector2i GROUPS = GROUP_SIZE;
                constexpr int GROUPS_X = GROUP_SIZE[0];
                constexpr int GROUPS_Y = GROUP_SIZE[1];

                grayscale_groups_ = grayscale_groups(GROUPS, sizes);
                grayscale_program_.create_pipeline(GROUPS_X, GROUPS_Y, rectangle);
                grayscale_memory_.set_src(sampler, input.image_view());
                grayscale_memory_.set_dst(images_[0][0].image_view(), images_[1][0].image_view());

                downsample_groups_ = downsample_groups(GROUPS, sizes);
                downsample_program_.create_pipeline(GROUPS_X, GROUPS_Y);
                downsample_memory_ =
                        create_downsample_memory(*device_, downsample_program_.descriptor_set_layout(), images_);

                sobel_groups_ = sobel_groups(GROUPS, sizes);
                sobel_program_.create_pipeline(GROUPS_X, GROUPS_Y);
                sobel_memory_ =
                        create_sobel_memory(*device_, sobel_program_.descriptor_set_layout(), images_, dx_, dy_);

                flow_groups_ = flow_groups(GROUPS, sizes, top_point_count_x, top_point_count_y);
                flow_program_.create_pipeline(
                        GROUPS_X, GROUPS_Y, RADIUS, MAX_ITERATION_COUNT, STOP_MOVE_SQUARE, MIN_DETERMINANT);

                flow_memory_ = create_flow_memory(
                        *device_, flow_program_.descriptor_set_layout(), family_index, sampler, sizes,
                        to_buffer_pointers(flow_buffers_), top_point_count_x, top_point_count_y, top_points, top_flow,
                        images_, dx_, dy_);

                create_command_buffer_first_pyramid();
                create_command_buffers(top_flow);
        }

        void delete_buffers() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                command_buffer_first_pyramid_.reset();
                command_buffers_.reset();

                grayscale_program_.delete_pipeline();
                downsample_program_.delete_pipeline();
                sobel_program_.delete_pipeline();
                flow_program_.delete_pipeline();

                images_[0].clear();
                images_[1].clear();
                dx_.clear();
                dy_.clear();
                flow_buffers_.clear();
                downsample_memory_.clear();
                downsample_memory_.clear();
                sobel_memory_.clear();
                flow_memory_.clear();
        }

        void reset() override
        {
                i_index_ = -1;
        }

public:
        Impl(const vulkan::VulkanInstance* instance,
             const vulkan::CommandPool* compute_command_pool,
             const vulkan::Queue* compute_queue)
                : instance_(instance),
                  device_(&instance->device()),
                  compute_command_pool_(compute_command_pool),
                  compute_queue_(compute_queue),
                  semaphore_first_pyramid_(instance->device()),
                  semaphore_(instance->device()),
                  grayscale_program_(instance->device()),
                  grayscale_memory_(*device_, grayscale_program_.descriptor_set_layout()),
                  downsample_program_(instance->device()),
                  sobel_program_(instance->device()),
                  flow_program_(instance->device())
        {
                ASSERT(compute_command_pool->family_index() == compute_queue->family_index());
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                instance_->device_wait_idle_noexcept("the Vulkan optical flow compute destructor");
        }
};
}

vulkan::DeviceFeatures Compute::required_device_features()
{
        return device_features();
}

std::unique_ptr<Compute> create_compute(
        const vulkan::VulkanInstance* instance,
        const vulkan::CommandPool* compute_command_pool,
        const vulkan::Queue* compute_queue)
{
        return std::make_unique<Impl>(instance, compute_command_pool, compute_queue);
}
}
