/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "barriers.h"
#include "function.h"
#include "option.h"

#include "compute/flow.h"
#include "compute/image_pyramid.h"
#include "compute/sobel.h"

#include <src/com/error.h>
#include <src/numerical/region.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/queue.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

namespace ns::gpu::optical_flow
{
namespace
{
constexpr VkFormat IMAGE_FORMAT = VK_FORMAT_R32_SFLOAT;

std::vector<vulkan::ImageWithMemory> create_images(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<numerical::Vector2i>& sizes,
        const VkFormat format,
        const std::uint32_t family_index,
        const VkImageUsageFlags usage)
{
        const std::vector<std::uint32_t> family_indices({compute_command_pool.family_index(), family_index});
        const std::vector<VkFormat> formats({format});

        std::vector<vulkan::ImageWithMemory> res;
        res.reserve(sizes.size());
        for (const numerical::Vector2i& s : sizes)
        {
                res.emplace_back(
                        device, family_indices, formats, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(s[0], s[1]), usage, VK_IMAGE_LAYOUT_GENERAL, compute_command_pool,
                        compute_queue);
        }
        return res;
}

class Impl final : public Compute
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

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

        compute::ImagePyramid program_image_pyramid_;
        compute::Sobel program_sobel_;
        compute::Flow program_flow_;

        int i_index_ = -1;

        void commands_images_to_sampler_layout(const int index, const VkCommandBuffer command_buffer)
        {
                for (const vulkan::ImageWithMemory& image : images_[index])
                {
                        image_barrier(
                                command_buffer, image.image().handle(), VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, VK_ACCESS_SHADER_READ_BIT);
                }
        }

        void commands_images_to_general_layout(const int index, const VkCommandBuffer command_buffer)
        {
                for (const vulkan::ImageWithMemory& image : images_[index])
                {
                        image_barrier(
                                command_buffer, image.image().handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                VK_IMAGE_LAYOUT_GENERAL, 0, VK_ACCESS_SHADER_READ_BIT);
                }
        }

        void create_command_buffer_first_pyramid()
        {
                command_buffer_first_pyramid_ =
                        vulkan::handle::CommandBuffer(device_->handle(), compute_command_pool_->handle());

                const VkCommandBuffer command_buffer = *command_buffer_first_pyramid_;

                const auto commands = [&]
                {
                        program_image_pyramid_.commands(images_, 0, command_buffer);
                };

                vulkan::record_commands(command_buffer, commands);
        }

        void create_command_buffers(const VkBuffer top_flow)
        {
                command_buffers_ =
                        vulkan::handle::CommandBuffers(device_->handle(), compute_command_pool_->handle(), 2);

                for (int index = 0; index < 2; ++index)
                {
                        const VkCommandBuffer command_buffer = (*command_buffers_)[index];

                        const auto commands = [&]
                        {
                                // i — previous image, 1-i — current image
                                program_image_pyramid_.commands(images_, 1 - index, command_buffer);
                                program_sobel_.commands(dx_, dy_, index, command_buffer);

                                commands_images_to_sampler_layout(1 - index, command_buffer);
                                program_flow_.commands(index, command_buffer, top_flow);
                                commands_images_to_general_layout(1 - index, command_buffer);
                        };

                        vulkan::record_commands(command_buffer, commands);
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
                                semaphore_first_pyramid_, queue.handle());
                        wait_semaphore = semaphore_first_pyramid_;
                }
                else
                {
                        i_index_ = 1 - i_index_;
                }

                vulkan::queue_submit(
                        wait_semaphore, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, (*command_buffers_)[i_index_], semaphore_,
                        queue.handle());

                return semaphore_;
        }

        void create_buffers(
                const VkSampler sampler,
                const vulkan::ImageWithMemory& input,
                const numerical::Region<2, int>& rectangle,
                const unsigned top_point_count_x,
                const unsigned top_point_count_y,
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

                const std::vector<numerical::Vector2i> sizes = pyramid_sizes(
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

                program_image_pyramid_.create_buffers(sampler, input, rectangle, sizes, images_);

                program_sobel_.create_buffers(sizes, dx_, dy_, images_);

                program_flow_.create_buffers(
                        sampler, family_index, sizes, top_point_count_x, top_point_count_y, top_points, top_flow,
                        images_, dx_, dy_);

                create_command_buffer_first_pyramid();
                create_command_buffers(top_flow.handle());
        }

        void delete_buffers() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                command_buffer_first_pyramid_.reset();
                command_buffers_.reset();

                program_image_pyramid_.delete_buffers();
                program_sobel_.delete_buffers();
                program_flow_.delete_buffers();

                images_[0].clear();
                images_[1].clear();
                dx_.clear();
                dy_.clear();
        }

        void reset() override
        {
                i_index_ = -1;
        }

public:
        Impl(const vulkan::Device* const device,
             const vulkan::CommandPool* const compute_command_pool,
             const vulkan::Queue* const compute_queue)
                : device_(device),
                  compute_command_pool_(compute_command_pool),
                  compute_queue_(compute_queue),
                  semaphore_first_pyramid_(device_->handle()),
                  semaphore_(device_->handle()),
                  program_image_pyramid_(device_->handle()),
                  program_sobel_(device_->handle()),
                  program_flow_(device_)
        {
                ASSERT(compute_command_pool->family_index() == compute_queue->family_index());
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                device_->wait_idle_noexcept("optical flow compute destructor");
        }
};
}

std::unique_ptr<Compute> create_compute(
        const vulkan::Device* const device,
        const vulkan::CommandPool* const compute_command_pool,
        const vulkan::Queue* const compute_queue)
{
        return std::make_unique<Impl>(device, compute_command_pool, compute_queue);
}
}
