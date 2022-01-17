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

/*
Eleanor Chu, Alan George.
INSIDE the FFT BLACK BOX. Serial and Parallel Fast Fourier Transform Algorithms.
CRC Press LLC, 2000.

13. FFTs for Arbitrary N
*/

/*
There are errors in chapter 13 when calculating H2
  Example 13.4
    written:
      h0, h1, h2, h3, h4, h5, 0, 0, 0, 0, 0,  0, h4, h3, h2, h1.
    correct:
      h0, h1, h2, h3, h4, h5, 0, 0, 0, 0, 0, h5, h4, h3, h2, h1.

  Formulas 13.11, 13.23, 13.24, 13.25.
    Written:
      h2(l) = h(l), if l = 0,...,N - 1,
      h2(l) = 0, if l = N,..., M - N + 1,
      h2(l) = h(M - l), if l = M - N + 2,..., M - 1.
    Correct:
      h2(l) = h(l), if l = 0,...,N - 1,
      h2(l) = 0, if l = N,..., M - N,
      h2(l) = h(M - l), if l = M - N + 1,..., M - 1.
*/

#include "compute.h"

#include "barriers.h"
#include "dft.h"
#include "function.h"

#include "../com/groups.h"
#include "shaders/copy_input.h"
#include "shaders/copy_output.h"

#include <src/com/error.h>
#include <src/vulkan/device_instance.h>
#include <src/vulkan/error.h>
#include <src/vulkan/instance.h>
#include <src/vulkan/queue.h>

#include <optional>

namespace ns::gpu::dft
{
namespace
{
constexpr const Vector2i GROUP_SIZE_2D = Vector2i(16, 16);

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

class DftImage final : public ComputeImage
{
        std::unique_ptr<Dft> dft_;

        CopyInputProgram copy_input_program_;
        CopyInputMemory copy_input_memory_;
        CopyOutputProgram copy_output_program_;
        CopyOutputMemory copy_output_memory_;
        Vector2i copy_groups_ = Vector2i(0, 0);

        VkImage output_ = VK_NULL_HANDLE;

        void create_buffers(
                const VkSampler sampler,
                const vulkan::ImageWithMemory& input,
                const vulkan::ImageWithMemory& output,
                const Region<2, int>& rectangle,
                const std::uint32_t family_index) override
        {
                ASSERT(sampler != VK_NULL_HANDLE);

                ASSERT(output.image().type() == VK_IMAGE_TYPE_2D);
                ASSERT(input.image().type() == VK_IMAGE_TYPE_2D);

                ASSERT(rectangle.width() == static_cast<int>(output.image().extent().width));
                ASSERT(rectangle.height() == static_cast<int>(output.image().extent().height));
                ASSERT(rectangle.x1() <= static_cast<int>(input.image().extent().width));
                ASSERT(rectangle.y1() <= static_cast<int>(input.image().extent().height));

                dft_->create_buffers(rectangle.width(), rectangle.height(), family_index);

                //

                copy_input_memory_.set(sampler, input.image_view(), dft_->buffer());
                copy_input_program_.create_pipeline(GROUP_SIZE_2D[0], GROUP_SIZE_2D[1], rectangle);

                const int width = rectangle.width();
                const int height = rectangle.height();

                copy_output_memory_.set(dft_->buffer(), output.image_view());
                copy_output_program_.create_pipeline(GROUP_SIZE_2D[0], GROUP_SIZE_2D[1], 1.0 / (width * height));

                copy_groups_ = group_count(width, height, GROUP_SIZE_2D);

                output_ = output.image();
        }

        void delete_buffers() override
        {
                output_ = VK_NULL_HANDLE;

                copy_output_program_.delete_pipeline();
                copy_input_program_.delete_pipeline();

                //

                dft_->delete_buffers();
        }

        void compute_commands(const VkCommandBuffer command_buffer) const override
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, copy_input_program_.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, copy_input_program_.pipeline_layout(),
                        CopyInputMemory::set_number(), 1, &copy_input_memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, copy_groups_[0], copy_groups_[1], 1);

                buffer_barrier(command_buffer, dft_->buffer());

                //

                constexpr bool INVERSE = false;
                dft_->compute_commands(command_buffer, INVERSE);

                //

                image_barrier_before(command_buffer, output_);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, copy_output_program_.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, copy_output_program_.pipeline_layout(),
                        CopyOutputMemory::set_number(), 1, &copy_output_memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, copy_groups_[0], copy_groups_[1], 1);

                image_barrier_after(command_buffer, output_);
        }

public:
        DftImage(
                const vulkan::Device* const device,
                const vulkan::CommandPool* const compute_command_pool,
                const vulkan::Queue* const compute_queue,
                const vulkan::CommandPool* const transfer_command_pool,
                const vulkan::Queue* const transfer_queue)
                : dft_(create_dft(
                        device,
                        compute_command_pool,
                        compute_queue,
                        transfer_command_pool,
                        transfer_queue,
                        vulkan::BufferMemoryType::DEVICE_LOCAL,
                        GROUP_SIZE_2D)),
                  copy_input_program_(*device),
                  copy_input_memory_(*device, copy_input_program_.descriptor_set_layout()),
                  copy_output_program_(*device),
                  copy_output_memory_(*device, copy_output_program_.descriptor_set_layout())
        {
        }
};

class DftVector final : public ComputeVector
{
        vulkan::DeviceInstance instance_;

        std::unique_ptr<Dft> dft_;

        std::optional<vulkan::handle::CommandBuffers> command_buffers_;
        unsigned width_ = 0;
        unsigned height_ = 0;

        enum DftType
        {
                FORWARD,
                INVERSE
        };

        void delete_buffers()
        {
                width_ = -1;
                height_ = -1;

                command_buffers_.reset();
                dft_->delete_buffers();
        }

        void create_buffers(const unsigned width, const unsigned height) override
        {
                delete_buffers();

                //

                dft_->create_buffers(width, height, instance_.compute_queue().family_index());

                command_buffers_ =
                        vulkan::handle::CommandBuffers(instance_.device(), instance_.compute_command_pool(), 2);

                for (const int index : {DftType::FORWARD, DftType::INVERSE})
                {
                        const VkCommandBuffer command_buffer = (*command_buffers_)[index];

                        begin_command_buffer(command_buffer);

                        const bool inverse = (index == DftType::INVERSE);
                        dft_->compute_commands(command_buffer, inverse);

                        end_command_buffer(command_buffer);
                }

                width_ = width;
                height_ = height;
        }

        void exec(const bool inverse, std::vector<std::complex<float>>* const src) override
        {
                if (!(width_ > 0 && height_ > 0 && command_buffers_))
                {
                        error("No DFT buffers");
                }

                if (!(src && (src->size() == static_cast<std::size_t>(width_) * height_)))
                {
                        error("Wrong DFT buffer size");
                }

                {
                        vulkan::BufferMapper mapper(dft_->buffer_with_memory());
                        mapper.write(*src);
                }

                vulkan::queue_submit(
                        (*command_buffers_)[inverse ? DftType::INVERSE : DftType::FORWARD], instance_.compute_queue());
                VULKAN_CHECK(vkQueueWaitIdle(instance_.compute_queue()));

                {
                        vulkan::BufferMapper mapper(dft_->buffer_with_memory());
                        mapper.read(src);
                }
        }

public:
        DftVector()
                : instance_(vulkan::Instance::handle()),
                  dft_(create_dft(
                          &instance_.device(),
                          &instance_.compute_command_pool(),
                          &instance_.compute_queue(),
                          &instance_.transfer_command_pool(),
                          &instance_.transfer_queue(),
                          vulkan::BufferMemoryType::HOST_VISIBLE,
                          GROUP_SIZE_2D))
        {
        }
};
}

std::unique_ptr<ComputeImage> create_compute_image(
        const vulkan::Device* device,
        const vulkan::CommandPool* compute_command_pool,
        const vulkan::Queue* compute_queue,
        const vulkan::CommandPool* transfer_command_pool,
        const vulkan::Queue* transfer_queue)
{
        return std::make_unique<DftImage>(
                device, compute_command_pool, compute_queue, transfer_command_pool, transfer_queue);
}

std::unique_ptr<ComputeVector> create_compute_vector()
{
        return std::make_unique<DftVector>();
}
}
