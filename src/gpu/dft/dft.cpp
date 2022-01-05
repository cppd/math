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
*/

#include "dft.h"

#include "barriers.h"
#include "buffer.h"
#include "fft.h"

#include "../com/groups.h"
#include "shaders/mul.h"
#include "shaders/mul_d.h"

#include <src/com/error.h>

#include <thread>

namespace ns::gpu::dft
{
namespace
{
class Impl final : public Dft
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const vulkan::VulkanInstance* const instance_;
        const vulkan::Device* const device_;

        const vulkan::CommandPool* const compute_command_pool_;
        const vulkan::Queue* const compute_queue_;
        const vulkan::CommandPool* const transfer_command_pool_;
        const vulkan::Queue* const transfer_queue_;

        const vulkan::BufferMemoryType buffer_memory_type_;

        const Vector2i group_size_;

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

        void create_diagonals(const std::uint32_t family_index)
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

                const std::vector<std::uint32_t> family_indices = {
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

        //

        void create_buffers(const unsigned width, const unsigned height, const std::uint32_t family_index) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                ASSERT(width > 0 && height > 0);

                n1_ = width;
                n2_ = height;
                m1_ = compute_m(n1_);
                m2_ = compute_m(n2_);

                create_diagonals(family_index);

                const std::vector<std::uint32_t> family_indices = {family_index};

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
                mul_program_.create_pipelines(n1_, n2_, m1_, m2_, group_size_[0], group_size_[1]);
                mul_rows_to_buffer_groups_ = group_count(m1_, n2_, group_size_);
                mul_rows_from_buffer_groups_ = group_count(n1_, n2_, group_size_);
                mul_columns_to_buffer_groups_ = group_count(n1_, m2_, group_size_);
                mul_columns_from_buffer_groups_ = group_count(n1_, n2_, group_size_);

                mul_d_d1_fwd_.set(d1_fwd_->buffer(), buffer_->buffer());
                mul_d_d1_inv_.set(d1_inv_->buffer(), buffer_->buffer());
                mul_d_d2_fwd_.set(d2_fwd_->buffer(), buffer_->buffer());
                mul_d_d2_inv_.set(d2_inv_->buffer(), buffer_->buffer());
                mul_d_program_.create_pipelines(n1_, n2_, m1_, m2_, group_size_[0], group_size_[1]);
                mul_d_row_groups_ = group_count(m1_, n2_, group_size_);
                mul_d_column_groups_ = group_count(m2_, n1_, group_size_);
        }

        void delete_buffers() override
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

        void compute_commands(const VkCommandBuffer command_buffer, const bool inverse) const override
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

        const vulkan::Buffer& buffer() const override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(x_d_);
                return x_d_->buffer();
        }

        const vulkan::BufferWithMemory& buffer_with_memory() const override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(x_d_);
                return x_d_->buffer_with_memory();
        }

public:
        Impl(const vulkan::VulkanInstance* const instance,
             const vulkan::CommandPool* const compute_command_pool,
             const vulkan::Queue* const compute_queue,
             const vulkan::CommandPool* const transfer_command_pool,
             const vulkan::Queue* const transfer_queue,
             const vulkan::BufferMemoryType buffer_memory_type,
             const Vector2i& group_size)
                : instance_(instance),
                  device_(&instance->device()),
                  compute_command_pool_(compute_command_pool),
                  compute_queue_(compute_queue),
                  transfer_command_pool_(transfer_command_pool),
                  transfer_queue_(transfer_queue),
                  buffer_memory_type_(buffer_memory_type),
                  group_size_(group_size),
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

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                instance_->device_wait_idle_noexcept("the Vulkan DFT compute destructor");
        }
};
}

std::unique_ptr<Dft> create_dft(
        const vulkan::VulkanInstance* const instance,
        const vulkan::CommandPool* const compute_command_pool,
        const vulkan::Queue* const compute_queue,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        const vulkan::BufferMemoryType buffer_memory_type,
        const Vector2i& group_size)
{
        return std::make_unique<Impl>(
                instance, compute_command_pool, compute_queue, transfer_command_pool, transfer_queue,
                buffer_memory_type, group_size);
}
}
