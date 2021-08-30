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

#include "function.h"
#include "option.h"

#include "shaders/downsample.h"
#include "shaders/flow.h"
#include "shaders/grayscale.h"
#include "shaders/sobel.h"

#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>

#include <thread>

namespace ns::gpu::optical_flow
{
namespace
{
constexpr VkFormat IMAGE_FORMAT = VK_FORMAT_R32_SFLOAT;

constexpr vulkan::DeviceFeatures REQUIRED_DEVICE_FEATURES = {};

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

class Impl final : public Compute
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const vulkan::VulkanInstance& instance_;
        const vulkan::Device& device_;

        const vulkan::CommandPool& compute_command_pool_;
        const vulkan::Queue& compute_queue_;

        vulkan::Semaphore semaphore_first_pyramid_;
        vulkan::Semaphore semaphore_;

        std::optional<vulkan::CommandBuffer> command_buffer_first_pyramid_;
        std::optional<vulkan::CommandBuffers> command_buffers_;

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

        std::vector<vulkan::ImageWithMemory> create_images(
                const std::vector<Vector2i>& sizes,
                uint32_t family_index,
                VkImageUsageFlags usage) const
        {
                std::vector<vulkan::ImageWithMemory> images;
                images.reserve(sizes.size());

                const std::vector<uint32_t> family_indices({compute_command_pool_.family_index(), family_index});
                const std::vector<VkFormat> formats({IMAGE_FORMAT});
                for (const Vector2i& s : sizes)
                {
                        images.emplace_back(
                                device_, family_indices, formats, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                                vulkan::make_extent(s[0], s[1]), usage, VK_IMAGE_LAYOUT_GENERAL, compute_command_pool_,
                                compute_queue_);
                }

                return images;
        }

        std::vector<vulkan::BufferWithMemory> create_flow_buffers(
                const std::vector<Vector2i>& sizes,
                uint32_t family_index) const
        {
                std::vector<vulkan::BufferWithMemory> buffers;
                if (sizes.size() <= 1)
                {
                        return {};
                }
                buffers.reserve(sizes.size() - 1);

                const std::vector<uint32_t> family_indices({family_index});
                for (std::size_t i = 1; i < sizes.size(); ++i)
                {
                        const int buffer_size = sizes[i][0] * sizes[i][1] * sizeof(Vector2f);
                        buffers.emplace_back(
                                vulkan::BufferMemoryType::DEVICE_LOCAL, device_, family_indices,
                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size);
                }

                return buffers;
        }

        static std::vector<DownsampleMemory> create_downsample_memory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images)
        {
                ASSERT(images[0].size() == images[1].size());

                std::vector<DownsampleMemory> downsample_images;

                for (unsigned i = 1; i < images[0].size(); ++i)
                {
                        downsample_images.emplace_back(device, descriptor_set_layout);
                        downsample_images.back().set_big(images[0][i - 1], images[1][i - 1]);
                        downsample_images.back().set_small(images[0][i], images[1][i]);
                }

                return downsample_images;
        }

        static std::vector<SobelMemory> create_sobel_memory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images,
                const std::vector<vulkan::ImageWithMemory>& dx,
                const std::vector<vulkan::ImageWithMemory>& dy)
        {
                ASSERT(images[0].size() == images[1].size());
                ASSERT(images[0].size() == dx.size());
                ASSERT(images[0].size() == dy.size());

                std::vector<SobelMemory> sobel_images;

                for (std::size_t i = 0; i < images[0].size(); ++i)
                {
                        sobel_images.emplace_back(device, descriptor_set_layout);
                        sobel_images.back().set_i(images[0][i], images[1][i]);
                        sobel_images.back().set_dx(dx[i]);
                        sobel_images.back().set_dy(dy[i]);
                }

                return sobel_images;
        }

        static std::vector<FlowMemory> create_flow_memory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                uint32_t family_index,
                VkSampler sampler,
                const std::vector<Vector2i>& sizes,
                const std::vector<vulkan::BufferWithMemory>& flow_buffers,
                int top_point_count_x,
                int top_point_count_y,
                const vulkan::BufferWithMemory& top_points,
                const vulkan::BufferWithMemory& top_flow,
                const std::array<std::vector<vulkan::ImageWithMemory>, 2>& images,
                const std::vector<vulkan::ImageWithMemory>& dx,
                const std::vector<vulkan::ImageWithMemory>& dy)
        {
                const std::size_t size = sizes.size();

                if (size <= 1)
                {
                        return {};
                }

                ASSERT(images[0].size() == size);
                ASSERT(images[1].size() == size);
                ASSERT(dx.size() == size);
                ASSERT(dy.size() == size);

                ASSERT(flow_buffers.size() + 1 == size);
                const auto flow_index = [&](std::size_t i)
                {
                        ASSERT(i > 0 && i < size);
                        return i - 1; // buffers start at level 1
                };

                const std::vector<uint32_t> family_indices{family_index};

                std::vector<FlowMemory> flow_memory;

                for (std::size_t i = 0; i < size; ++i)
                {
                        const vulkan::BufferWithMemory* top_points_ptr;
                        const vulkan::BufferWithMemory* flow_ptr;
                        const vulkan::BufferWithMemory* flow_guess_ptr;

                        FlowMemory::Data data;

                        const bool top = (i == 0);
                        const bool bottom = (i + 1 == size);

                        if (!top)
                        {
                                top_points_ptr = &top_points; // not used
                                flow_ptr = &flow_buffers[flow_index(i)];
                                data.use_all_points = true;
                                data.point_count_x = sizes[i][0];
                                data.point_count_y = sizes[i][1];
                        }
                        else
                        {
                                top_points_ptr = &top_points;
                                flow_ptr = &top_flow;
                                data.use_all_points = false;
                                data.point_count_x = top_point_count_x;
                                data.point_count_y = top_point_count_y;
                        }

                        if (!bottom)
                        {
                                int i_prev = i + 1;
                                data.use_guess = true;
                                data.guess_kx = (sizes[i_prev][0] != sizes[i][0]) ? 2 : 1;
                                data.guess_ky = (sizes[i_prev][1] != sizes[i][1]) ? 2 : 1;
                                data.guess_width = sizes[i_prev][0];
                                flow_guess_ptr = &flow_buffers[flow_index(i_prev)];
                        }
                        else
                        {
                                flow_guess_ptr = &flow_buffers[0]; // not used
                                data.use_guess = false;
                        }

                        flow_memory.emplace_back(device, descriptor_set_layout, family_indices);

                        flow_memory[i].set_data(data);

                        flow_memory[i].set_top_points(*top_points_ptr);
                        flow_memory[i].set_flow(*flow_ptr);
                        flow_memory[i].set_flow_guess(*flow_guess_ptr);

                        flow_memory[i].set_dx(dx[i]);
                        flow_memory[i].set_dy(dy[i]);
                        flow_memory[i].set_i(images[0][i], images[1][i]);
                        flow_memory[i].set_j(sampler, images[1][i], images[0][i]);
                }

                return flow_memory;
        }

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
                                command_buffer, (i != 0) ? flow_buffers_[i - 1] : top_flow,
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
                VkResult result;

                command_buffer_first_pyramid_ = vulkan::CommandBuffer(device_, compute_command_pool_);

                VkCommandBuffer command_buffer = *command_buffer_first_pyramid_;

                VkCommandBufferBeginInfo command_buffer_info = {};
                command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
                }

                //

                commands_compute_image_pyramid(0, command_buffer);

                //

                result = vkEndCommandBuffer(command_buffer);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkEndCommandBuffer", result);
                }
        }

        void create_command_buffers(VkBuffer top_flow)
        {
                VkResult result;

                command_buffers_ = vulkan::CommandBuffers(device_, compute_command_pool_, 2);

                for (int index = 0; index < 2; ++index)
                {
                        VkCommandBuffer command_buffer = (*command_buffers_)[index];

                        VkCommandBufferBeginInfo command_buffer_info = {};
                        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                        result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
                        if (result != VK_SUCCESS)
                        {
                                vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
                        }

                        //

                        // i — previous image, 1-i — current image
                        commands_compute_image_pyramid(1 - index, command_buffer);
                        commands_compute_dxdy(index, command_buffer);

                        commands_images_to_sampler_layout(1 - index, command_buffer);
                        commands_compute_optical_flow(index, command_buffer, top_flow);
                        commands_images_to_general_layout(1 - index, command_buffer);

                        //

                        result = vkEndCommandBuffer(command_buffer);
                        if (result != VK_SUCCESS)
                        {
                                vulkan::vulkan_function_error("vkEndCommandBuffer", result);
                        }
                }
        }

        VkSemaphore compute(const vulkan::Queue& queue, VkSemaphore wait_semaphore) override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                ASSERT(queue.family_index() == compute_command_pool_.family_index());
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
                const vulkan::BufferWithMemory& top_points,
                const vulkan::BufferWithMemory& top_flow) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                ASSERT(sampler != VK_NULL_HANDLE);

                ASSERT(rectangle.is_positive());
                ASSERT(rectangle.x1() <= static_cast<int>(input.width()));
                ASSERT(rectangle.y1() <= static_cast<int>(input.height()));

                const std::vector<Vector2i> sizes =
                        pyramid_sizes(input.width(), input.height(), BOTTOM_IMAGE_MINIMUM_SIZE);

                const uint32_t family_index = compute_command_pool_.family_index();

                images_[0] =
                        create_images(sizes, family_index, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                images_[1] =
                        create_images(sizes, family_index, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                dx_ = create_images(sizes, family_index, VK_IMAGE_USAGE_STORAGE_BIT);
                dy_ = create_images(sizes, family_index, VK_IMAGE_USAGE_STORAGE_BIT);
                flow_buffers_ = create_flow_buffers(sizes, family_index);

                constexpr Vector2i GROUPS = GROUP_SIZE;
                constexpr int GROUPS_X = GROUP_SIZE[0];
                constexpr int GROUPS_Y = GROUP_SIZE[1];

                grayscale_groups_ = grayscale_groups(GROUPS, sizes);
                grayscale_program_.create_pipeline(GROUPS_X, GROUPS_Y, rectangle);
                grayscale_memory_.set_src(sampler, input);
                grayscale_memory_.set_dst(images_[0][0], images_[1][0]);

                downsample_groups_ = downsample_groups(GROUPS, sizes);
                downsample_program_.create_pipeline(GROUPS_X, GROUPS_Y);
                downsample_memory_ =
                        create_downsample_memory(device_, downsample_program_.descriptor_set_layout(), images_);

                sobel_groups_ = sobel_groups(GROUPS, sizes);
                sobel_program_.create_pipeline(GROUPS_X, GROUPS_Y);
                sobel_memory_ = create_sobel_memory(device_, sobel_program_.descriptor_set_layout(), images_, dx_, dy_);

                flow_groups_ = flow_groups(GROUPS, sizes, top_point_count_x, top_point_count_y);
                flow_program_.create_pipeline(
                        GROUPS_X, GROUPS_Y, RADIUS, MAX_ITERATION_COUNT, STOP_MOVE_SQUARE, MIN_DETERMINANT);
                flow_memory_ = create_flow_memory(
                        device_, flow_program_.descriptor_set_layout(), family_index, sampler, sizes, flow_buffers_,
                        top_point_count_x, top_point_count_y, top_points, top_flow, images_, dx_, dy_);

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
        Impl(const vulkan::VulkanInstance& instance,
             const vulkan::CommandPool& compute_command_pool,
             const vulkan::Queue& compute_queue)
                : instance_(instance),
                  device_(instance.device()),
                  compute_command_pool_(compute_command_pool),
                  compute_queue_(compute_queue),
                  semaphore_first_pyramid_(instance.device()),
                  semaphore_(instance.device()),
                  grayscale_program_(instance.device()),
                  grayscale_memory_(device_, grayscale_program_.descriptor_set_layout()),
                  downsample_program_(instance.device()),
                  sobel_program_(instance.device()),
                  flow_program_(instance.device())
        {
                ASSERT(compute_command_pool.family_index() == compute_queue.family_index());
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                instance_.device_wait_idle_noexcept("the Vulkan optical flow compute destructor");
        }
};
}

vulkan::DeviceFeatures Compute::required_device_features()
{
        return REQUIRED_DEVICE_FEATURES;
}

std::unique_ptr<Compute> create_compute(
        const vulkan::VulkanInstance& instance,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue)
{
        return std::make_unique<Impl>(instance, compute_command_pool, compute_queue);
}
}
