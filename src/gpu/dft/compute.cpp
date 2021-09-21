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

#include "buffer.h"
#include "fft.h"
#include "function.h"

#include "../com/groups.h"
#include "shaders/copy_input.h"
#include "shaders/copy_output.h"
#include "shaders/mul.h"
#include "shaders/mul_d.h"

#include <src/com/error.h>
#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>
#include <src/vulkan/sync.h>

#include <optional>
#include <thread>

namespace ns::gpu::dft
{
namespace
{
constexpr const Vector2i GROUP_SIZE_2D = Vector2i(16, 16);

vulkan::DeviceFeatures image_device_features()
{
        return {};
}
vulkan::DeviceFeatures vector_required_device_features()
{
        return {};
}
vulkan::DeviceFeatures vector_optional_device_features()
{
        return {};
}

void buffer_barrier(const VkCommandBuffer command_buffer, const VkBuffer buffer)
{
        ASSERT(command_buffer != VK_NULL_HANDLE);
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
                command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &barrier, 0, nullptr);
}

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

class Dft final
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const vulkan::VulkanInstance* const instance_;
        const vulkan::Device* const device_;

        const vulkan::CommandPool* const compute_command_pool_;
        const vulkan::Queue* const compute_queue_;
        const vulkan::CommandPool* const transfer_command_pool_;
        const vulkan::Queue* const transfer_queue_;

        const vulkan::BufferMemoryType buffer_memory_type_;

        MulProgram mul_program_;
        MulMemory mul_memory_;
        Vector2i mul_rows_to_buffer_groups_ = Vector2i(0, 0);
        Vector2i mul_rows_from_buffer_groups_ = Vector2i(0, 0);
        Vector2i mul_columns_to_buffer_groups_ = Vector2i(0, 0);
        Vector2i mul_columns_from_buffer_groups_ = Vector2i(0, 0);

        MulDProgram mul_d_program_;
        MulDMemory mul_d_d1_fwd_;
        MulDMemory mul_d_d1_inv_;
        MulDMemory mul_d_d2_fwd_;
        MulDMemory mul_d_d2_inv_;
        Vector2i mul_d_row_groups_ = Vector2i(0, 0);
        Vector2i mul_d_column_groups_ = Vector2i(0, 0);

        std::unique_ptr<Fft> fft_n2_m1_;
        std::unique_ptr<Fft> fft_n1_m2_;

        int n1_ = -1;
        int n2_ = -1;
        int m1_ = -1;
        int m2_ = -1;

        std::optional<ComplexNumberBuffer> d1_fwd_;
        std::optional<ComplexNumberBuffer> d1_inv_;
        std::optional<ComplexNumberBuffer> d2_fwd_;
        std::optional<ComplexNumberBuffer> d2_inv_;
        std::optional<ComplexNumberBuffer> x_d_;
        std::optional<ComplexNumberBuffer> buffer_;

        void rows_to_buffer(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_program_.pipeline_rows_to_buffer(inverse));
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_program_.pipeline_layout(),
                        MulMemory::set_number(), 1, &mul_memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, mul_rows_to_buffer_groups_[0], mul_rows_to_buffer_groups_[1], 1);

                buffer_barrier(command_buffer, buffer_->buffer());
        }

        void rows_mul_d(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                const VkDescriptorSet* set =
                        inverse ? &mul_d_d1_inv_.descriptor_set() : &mul_d_d1_fwd_.descriptor_set();
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_d_program_.pipeline_rows());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_d_program_.pipeline_layout(),
                        MulDMemory::set_number(), 1, set, 0, nullptr);
                vkCmdDispatch(command_buffer, mul_d_row_groups_[0], mul_d_row_groups_[1], 1);

                buffer_barrier(command_buffer, buffer_->buffer());
        }

        void rows_from_buffer(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                        mul_program_.pipeline_rows_from_buffer(inverse));
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_program_.pipeline_layout(),
                        MulMemory::set_number(), 1, &mul_memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, mul_rows_from_buffer_groups_[0], mul_rows_from_buffer_groups_[1], 1);

                buffer_barrier(command_buffer, x_d_->buffer());
        }

        void columns_to_buffer(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                        mul_program_.pipeline_columns_to_buffer(inverse));
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_program_.pipeline_layout(),
                        MulMemory::set_number(), 1, &mul_memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, mul_columns_to_buffer_groups_[0], mul_columns_to_buffer_groups_[1], 1);

                buffer_barrier(command_buffer, buffer_->buffer());
        }

        void columns_mul_d(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                const VkDescriptorSet* set =
                        inverse ? &mul_d_d2_inv_.descriptor_set() : &mul_d_d2_fwd_.descriptor_set();
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_d_program_.pipeline_columns());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_d_program_.pipeline_layout(),
                        MulDMemory::set_number(), 1, set, 0, nullptr);
                vkCmdDispatch(command_buffer, mul_d_column_groups_[0], mul_d_column_groups_[1], 1);

                buffer_barrier(command_buffer, buffer_->buffer());
        }

        void columns_from_buffer(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                        mul_program_.pipeline_columns_from_buffer(inverse));
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_program_.pipeline_layout(),
                        MulMemory::set_number(), 1, &mul_memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(
                        command_buffer, mul_columns_from_buffer_groups_[0], mul_columns_from_buffer_groups_[1], 1);

                buffer_barrier(command_buffer, x_d_->buffer());
        }

        void create_diagonals(const uint32_t family_index)
        {
                // Compute the diagonal D in Lemma 13.2: use the radix-2 FFT
                // 13.13, 13.26.

                // Coefficients for inverse DFT, because N is not equal to M.
                const double m1_div_n1 = static_cast<double>(m1_) / n1_;
                const double m2_div_n2 = static_cast<double>(m2_) / n2_;

                std::vector<std::complex<double>> d1_fwd = compute_h2(n1_, m1_, compute_h(n1_, false, 1.0));
                std::vector<std::complex<double>> d1_inv = compute_h2(n1_, m1_, compute_h(n1_, true, m1_div_n1));
                std::vector<std::complex<double>> d2_fwd = compute_h2(n2_, m2_, compute_h(n2_, false, 1.0));
                std::vector<std::complex<double>> d2_inv = compute_h2(n2_, m2_, compute_h(n2_, true, m2_div_n2));

                //

                const std::vector<uint32_t> family_indices = {
                        family_index, compute_command_pool_->family_index(), transfer_command_pool_->family_index()};

                d1_fwd_.emplace(*device_, *transfer_command_pool_, *transfer_queue_, family_indices, d1_fwd);
                d1_inv_.emplace(*device_, *transfer_command_pool_, *transfer_queue_, family_indices, d1_inv);
                d2_fwd_.emplace(*device_, *transfer_command_pool_, *transfer_queue_, family_indices, d2_fwd);
                d2_inv_.emplace(*device_, *transfer_command_pool_, *transfer_queue_, family_indices, d2_inv);

                {
                        std::unique_ptr<Fft> fft =
                                create_fft(instance_->device(), {compute_command_pool_->family_index()}, 1, m1_);
                        fft->run_for_data(false, *d1_fwd_, *device_, *compute_command_pool_, *compute_queue_);
                        fft->run_for_data(true, *d1_inv_, *device_, *compute_command_pool_, *compute_queue_);
                }
                {
                        std::unique_ptr<Fft> fft =
                                create_fft(instance_->device(), {compute_command_pool_->family_index()}, 1, m2_);
                        fft->run_for_data(false, *d2_fwd_, *device_, *compute_command_pool_, *compute_queue_);
                        fft->run_for_data(true, *d2_inv_, *device_, *compute_command_pool_, *compute_queue_);
                }
        }

public:
        void create_buffers(const unsigned width, const unsigned height, const uint32_t family_index)
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                ASSERT(width > 0 && height > 0);

                n1_ = width;
                n2_ = height;
                m1_ = compute_m(n1_);
                m2_ = compute_m(n2_);

                create_diagonals(family_index);

                const std::vector<uint32_t> family_indices = {family_index};

                x_d_.emplace(*device_, family_indices, n1_ * n2_, buffer_memory_type_);
                buffer_.emplace(
                        *device_, family_indices, std::max(m1_ * n2_, m2_ * n1_),
                        vulkan::BufferMemoryType::DEVICE_LOCAL);

                fft_n2_m1_.reset();
                fft_n2_m1_ = create_fft(instance_->device(), family_indices, n2_, m1_);
                fft_n2_m1_->set_data(*buffer_);

                fft_n1_m2_.reset();
                fft_n1_m2_ = create_fft(instance_->device(), family_indices, n1_, m2_);
                fft_n1_m2_->set_data(*buffer_);

                mul_memory_.set(x_d_->buffer(), buffer_->buffer());
                mul_program_.create_pipelines(n1_, n2_, m1_, m2_, GROUP_SIZE_2D[0], GROUP_SIZE_2D[1]);
                mul_rows_to_buffer_groups_ = group_count(m1_, n2_, GROUP_SIZE_2D);
                mul_rows_from_buffer_groups_ = group_count(n1_, n2_, GROUP_SIZE_2D);
                mul_columns_to_buffer_groups_ = group_count(n1_, m2_, GROUP_SIZE_2D);
                mul_columns_from_buffer_groups_ = group_count(n1_, n2_, GROUP_SIZE_2D);

                mul_d_d1_fwd_.set(d1_fwd_->buffer(), buffer_->buffer());
                mul_d_d1_inv_.set(d1_inv_->buffer(), buffer_->buffer());
                mul_d_d2_fwd_.set(d2_fwd_->buffer(), buffer_->buffer());
                mul_d_d2_inv_.set(d2_inv_->buffer(), buffer_->buffer());
                mul_d_program_.create_pipelines(n1_, n2_, m1_, m2_, GROUP_SIZE_2D[0], GROUP_SIZE_2D[1]);
                mul_d_row_groups_ = group_count(m1_, n2_, GROUP_SIZE_2D);
                mul_d_column_groups_ = group_count(m2_, n1_, GROUP_SIZE_2D);
        }

        void delete_buffers()
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                mul_program_.delete_pipelines();
                mul_d_program_.delete_pipelines();

                fft_n2_m1_.reset();
                fft_n1_m2_.reset();

                d1_fwd_.reset();
                d1_inv_.reset();
                d2_fwd_.reset();
                d2_inv_.reset();
                x_d_.reset();
                buffer_.reset();

                n1_ = n2_ = m1_ = m2_ = -1;
        }

        void compute_commands(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                if (n1_ > 1)
                {
                        rows_to_buffer(command_buffer, inverse);
                        fft_n2_m1_->commands(command_buffer, inverse);
                        rows_mul_d(command_buffer, inverse);
                        fft_n2_m1_->commands(command_buffer, !inverse);
                        rows_from_buffer(command_buffer, inverse);
                }
                if (n2_ > 1)
                {
                        columns_to_buffer(command_buffer, inverse);
                        fft_n1_m2_->commands(command_buffer, inverse);
                        columns_mul_d(command_buffer, inverse);
                        fft_n1_m2_->commands(command_buffer, !inverse);
                        columns_from_buffer(command_buffer, inverse);
                }
        }

        const vulkan::Buffer& buffer() const
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(x_d_);
                return x_d_->buffer();
        }

        const vulkan::BufferWithMemory& buffer_with_memory() const
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(x_d_);
                return x_d_->buffer_with_memory();
        }

        Dft(const vulkan::VulkanInstance* instance,
            const vulkan::CommandPool* compute_command_pool,
            const vulkan::Queue* compute_queue,
            const vulkan::CommandPool* transfer_command_pool,
            const vulkan::Queue* transfer_queue,
            const vulkan::BufferMemoryType& buffer_memory_type)
                : instance_(instance),
                  device_(&instance->device()),
                  compute_command_pool_(compute_command_pool),
                  compute_queue_(compute_queue),
                  transfer_command_pool_(transfer_command_pool),
                  transfer_queue_(transfer_queue),
                  buffer_memory_type_(buffer_memory_type),
                  mul_program_(instance->device()),
                  mul_memory_(instance->device(), mul_program_.descriptor_set_layout()),
                  mul_d_program_(instance->device()),
                  mul_d_d1_fwd_(instance->device(), mul_d_program_.descriptor_set_layout()),
                  mul_d_d1_inv_(instance->device(), mul_d_program_.descriptor_set_layout()),
                  mul_d_d2_fwd_(instance->device(), mul_d_program_.descriptor_set_layout()),
                  mul_d_d2_inv_(instance->device(), mul_d_program_.descriptor_set_layout())
        {
                ASSERT(compute_command_pool->family_index() == compute_queue->family_index());
                ASSERT(transfer_command_pool->family_index() == transfer_queue->family_index());
        }

        ~Dft()
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                //

                instance_->device_wait_idle_noexcept("the Vulkan DFT compute destructor");
        }
};

class DftImage final : public ComputeImage
{
        Dft dft_;

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
                const uint32_t family_index) override
        {
                ASSERT(sampler != VK_NULL_HANDLE);

                ASSERT(rectangle.width() == static_cast<int>(output.width()));
                ASSERT(rectangle.height() == static_cast<int>(output.height()));
                ASSERT(rectangle.x1() <= static_cast<int>(input.width()));
                ASSERT(rectangle.y1() <= static_cast<int>(input.height()));

                dft_.create_buffers(rectangle.width(), rectangle.height(), family_index);

                //

                copy_input_memory_.set(sampler, input, dft_.buffer());
                copy_input_program_.create_pipeline(GROUP_SIZE_2D[0], GROUP_SIZE_2D[1], rectangle);

                const int width = rectangle.width();
                const int height = rectangle.height();

                copy_output_memory_.set(dft_.buffer(), output);
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

                dft_.delete_buffers();
        }

        void compute_commands(const VkCommandBuffer command_buffer) const override
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, copy_input_program_.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, copy_input_program_.pipeline_layout(),
                        CopyInputMemory::set_number(), 1, &copy_input_memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, copy_groups_[0], copy_groups_[1], 1);

                buffer_barrier(command_buffer, dft_.buffer());

                //

                constexpr bool INVERSE = false;
                dft_.compute_commands(command_buffer, INVERSE);

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
                const vulkan::VulkanInstance* instance,
                const vulkan::CommandPool* compute_command_pool,
                const vulkan::Queue* compute_queue,
                const vulkan::CommandPool* transfer_command_pool,
                const vulkan::Queue* transfer_queue)
                : dft_(instance,
                       compute_command_pool,
                       compute_queue,
                       transfer_command_pool,
                       transfer_queue,
                       vulkan::BufferMemoryType::DEVICE_LOCAL),
                  copy_input_program_(instance->device()),
                  copy_input_memory_(instance->device(), copy_input_program_.descriptor_set_layout()),
                  copy_output_program_(instance->device()),
                  copy_output_memory_(instance->device(), copy_output_program_.descriptor_set_layout())
        {
        }
};

class DftVector final : public ComputeVector
{
        vulkan::VulkanInstance instance_;

        Dft dft_;

        std::optional<vulkan::CommandBuffers> command_buffers_;
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
                dft_.delete_buffers();
        }

        void create_buffers(const unsigned width, const unsigned height) override
        {
                delete_buffers();

                //

                dft_.create_buffers(width, height, instance_.compute_queue().family_index());

                command_buffers_ = vulkan::CommandBuffers(instance_.device(), instance_.compute_command_pool(), 2);
                VkResult result;
                for (int index : {DftType::FORWARD, DftType::INVERSE})
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

                        const bool inverse = (index == DftType::INVERSE);
                        dft_.compute_commands(command_buffer, inverse);

                        //

                        result = vkEndCommandBuffer(command_buffer);
                        if (result != VK_SUCCESS)
                        {
                                vulkan::vulkan_function_error("vkEndCommandBuffer", result);
                        }
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
                        vulkan::BufferMapper mapper(dft_.buffer_with_memory());
                        mapper.write(*src);
                }

                vulkan::queue_submit(
                        (*command_buffers_)[inverse ? DftType::INVERSE : DftType::FORWARD], instance_.compute_queue());
                vulkan::queue_wait_idle(instance_.compute_queue());

                {
                        vulkan::BufferMapper mapper(dft_.buffer_with_memory());
                        mapper.read(src);
                }
        }

public:
        DftVector()
                : instance_({}, {}, vector_required_device_features(), vector_optional_device_features()),
                  dft_(&instance_,
                       &instance_.compute_command_pool(),
                       &instance_.compute_queue(),
                       &instance_.transfer_command_pool(),
                       &instance_.transfer_queue(),
                       vulkan::BufferMemoryType::HOST_VISIBLE)
        {
        }
};
}

vulkan::DeviceFeatures ComputeImage::required_device_features()
{
        return image_device_features();
}

std::unique_ptr<ComputeImage> create_compute_image(
        const vulkan::VulkanInstance* instance,
        const vulkan::CommandPool* compute_command_pool,
        const vulkan::Queue* compute_queue,
        const vulkan::CommandPool* transfer_command_pool,
        const vulkan::Queue* transfer_queue)
{
        return std::make_unique<DftImage>(
                instance, compute_command_pool, compute_queue, transfer_command_pool, transfer_queue);
}

std::unique_ptr<ComputeVector> create_compute_vector()
{
        return std::make_unique<DftVector>();
}
}
