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

/*
Eleanor Chu, Alan George.
INSIDE the FFT BLACK BOX. Serial and Parallel Fast Fourier Transform Algorithms.
CRC Press LLC, 2000.
*/

#include "dft.h"

#include "barriers.h"
#include "buffer.h"
#include "fft.h"
#include "function.h"

#include "shaders/mul.h"
#include "shaders/mul_d.h"

#include <src/com/error.h>
#include <src/com/group_count.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <algorithm>
#include <complex>
#include <cstdint>
#include <memory>
#include <optional>
#include <thread>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace ns::gpu::dft
{
namespace
{
class Diagonals final
{
        static void run_fft(
                const ComplexNumberBuffer& fwd,
                const ComplexNumberBuffer& inv,
                const int n,
                const vulkan::Device& device,
                const vulkan::CommandPool& compute_command_pool,
                const vulkan::Queue& compute_queue)
        {
                const std::unique_ptr<Fft> fft = create_fft(device, {compute_command_pool.family_index()}, 1, n);

                fft->run_for_data(false, fwd, device.handle(), compute_command_pool.handle(), compute_queue.handle());
                fft->run_for_data(true, inv, device.handle(), compute_command_pool.handle(), compute_queue.handle());
        }

        std::optional<ComplexNumberBuffer> d1_fwd_;
        std::optional<ComplexNumberBuffer> d1_inv_;
        std::optional<ComplexNumberBuffer> d2_fwd_;
        std::optional<ComplexNumberBuffer> d2_inv_;

public:
        Diagonals(
                const int n_1,
                const int n_2,
                const int m_1,
                const int m_2,
                const vulkan::Device& device,
                const vulkan::CommandPool& compute_command_pool,
                const vulkan::Queue& compute_queue,
                const vulkan::CommandPool& transfer_command_pool,
                const vulkan::Queue& transfer_queue,
                const std::uint32_t family_index)
        {
                // Compute the diagonal D in Lemma 13.2: use the radix-2 FFT
                // 13.13, 13.26.

                // Coefficients for inverse DFT, because N is not equal to M
                const double d1_coef = static_cast<double>(m_1) / n_1;
                const double d2_coef = static_cast<double>(m_2) / n_2;

                const std::vector<std::complex<double>> d1_fwd = compute_h2(n_1, m_1, compute_h(n_1, false, 1));
                const std::vector<std::complex<double>> d1_inv = compute_h2(n_1, m_1, compute_h(n_1, true, d1_coef));
                const std::vector<std::complex<double>> d2_fwd = compute_h2(n_2, m_2, compute_h(n_2, false, 1));
                const std::vector<std::complex<double>> d2_inv = compute_h2(n_2, m_2, compute_h(n_2, true, d2_coef));

                const std::vector<std::uint32_t> family_indices = {
                        family_index, compute_command_pool.family_index(), transfer_command_pool.family_index()};

                d1_fwd_.emplace(device, transfer_command_pool, transfer_queue, family_indices, d1_fwd);
                d1_inv_.emplace(device, transfer_command_pool, transfer_queue, family_indices, d1_inv);
                d2_fwd_.emplace(device, transfer_command_pool, transfer_queue, family_indices, d2_fwd);
                d2_inv_.emplace(device, transfer_command_pool, transfer_queue, family_indices, d2_inv);

                run_fft(*d1_fwd_, *d1_inv_, m_1, device, compute_command_pool, compute_queue);
                run_fft(*d2_fwd_, *d2_inv_, m_2, device, compute_command_pool, compute_queue);
        }

        [[nodiscard]] const vulkan::Buffer& d1_fwd() const
        {
                return d1_fwd_->buffer();
        }

        [[nodiscard]] const vulkan::Buffer& d1_inv() const
        {
                return d1_inv_->buffer();
        }

        [[nodiscard]] const vulkan::Buffer& d2_fwd() const
        {
                return d2_fwd_->buffer();
        }

        [[nodiscard]] const vulkan::Buffer& d2_inv() const
        {
                return d2_inv_->buffer();
        }
};

class Impl final : public Dft
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

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

        unsigned width_;
        unsigned height_;

        std::optional<Diagonals> diagonals_;
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

                buffer_barrier(command_buffer, buffer_->buffer().handle());
        }

        void rows_mul_d(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                const VkDescriptorSet* const set =
                        inverse ? &mul_d_d1_inv_.descriptor_set() : &mul_d_d1_fwd_.descriptor_set();
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_d_program_.pipeline_rows());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_d_program_.pipeline_layout(),
                        MulDMemory::set_number(), 1, set, 0, nullptr);
                vkCmdDispatch(command_buffer, mul_d_row_groups_[0], mul_d_row_groups_[1], 1);

                buffer_barrier(command_buffer, buffer_->buffer().handle());
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

                buffer_barrier(command_buffer, x_d_->buffer().handle());
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

                buffer_barrier(command_buffer, buffer_->buffer().handle());
        }

        void columns_mul_d(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                const VkDescriptorSet* const set =
                        inverse ? &mul_d_d2_inv_.descriptor_set() : &mul_d_d2_fwd_.descriptor_set();
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_d_program_.pipeline_columns());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, mul_d_program_.pipeline_layout(),
                        MulDMemory::set_number(), 1, set, 0, nullptr);
                vkCmdDispatch(command_buffer, mul_d_column_groups_[0], mul_d_column_groups_[1], 1);

                buffer_barrier(command_buffer, buffer_->buffer().handle());
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

                buffer_barrier(command_buffer, x_d_->buffer().handle());
        }

        void create_buffers(const unsigned width, const unsigned height, const std::uint32_t family_index) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(width > 0 && height > 0);

                width_ = width;
                height_ = height;

                const int n_1 = width;
                const int n_2 = height;
                const int m_1 = compute_m(n_1);
                const int m_2 = compute_m(n_2);

                diagonals_.emplace(
                        n_1, n_2, m_1, m_2, *device_, *compute_command_pool_, *compute_queue_, *transfer_command_pool_,
                        *transfer_queue_, family_index);

                const std::vector<std::uint32_t> family_indices = {family_index};

                x_d_.emplace(*device_, family_indices, n_1 * n_2, buffer_memory_type_);
                buffer_.emplace(
                        *device_, family_indices, std::max(m_1 * n_2, m_2 * n_1),
                        vulkan::BufferMemoryType::DEVICE_LOCAL);

                fft_n2_m1_.reset();
                fft_n2_m1_ = create_fft(*device_, family_indices, n_2, m_1);
                fft_n2_m1_->set_data(*buffer_);

                fft_n1_m2_.reset();
                fft_n1_m2_ = create_fft(*device_, family_indices, n_1, m_2);
                fft_n1_m2_->set_data(*buffer_);

                mul_memory_.set(x_d_->buffer(), buffer_->buffer());
                mul_program_.create_pipelines(n_1, n_2, m_1, m_2, group_size_[0], group_size_[1]);
                mul_rows_to_buffer_groups_ = group_count({m_1, n_2}, group_size_);
                mul_rows_from_buffer_groups_ = group_count({n_1, n_2}, group_size_);
                mul_columns_to_buffer_groups_ = group_count({n_1, m_2}, group_size_);
                mul_columns_from_buffer_groups_ = group_count({n_1, n_2}, group_size_);

                mul_d_d1_fwd_.set(diagonals_->d1_fwd(), buffer_->buffer());
                mul_d_d1_inv_.set(diagonals_->d1_inv(), buffer_->buffer());
                mul_d_d2_fwd_.set(diagonals_->d2_fwd(), buffer_->buffer());
                mul_d_d2_inv_.set(diagonals_->d2_inv(), buffer_->buffer());
                mul_d_program_.create_pipelines(n_1, n_2, m_1, m_2, group_size_[0], group_size_[1]);
                mul_d_row_groups_ = group_count({m_1, n_2}, group_size_);
                mul_d_column_groups_ = group_count({m_2, n_1}, group_size_);
        }

        void delete_buffers() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                mul_program_.delete_pipelines();
                mul_d_program_.delete_pipelines();

                fft_n2_m1_.reset();
                fft_n1_m2_.reset();

                diagonals_.reset();
                x_d_.reset();
                buffer_.reset();
        }

        void compute_commands(const VkCommandBuffer command_buffer, const bool inverse) const override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                if (width_ > 1)
                {
                        rows_to_buffer(command_buffer, inverse);
                        fft_n2_m1_->commands(command_buffer, inverse);
                        rows_mul_d(command_buffer, inverse);
                        fft_n2_m1_->commands(command_buffer, !inverse);
                        rows_from_buffer(command_buffer, inverse);
                }

                if (height_ > 1)
                {
                        columns_to_buffer(command_buffer, inverse);
                        fft_n1_m2_->commands(command_buffer, inverse);
                        columns_mul_d(command_buffer, inverse);
                        fft_n1_m2_->commands(command_buffer, !inverse);
                        columns_from_buffer(command_buffer, inverse);
                }
        }

        [[nodiscard]] const vulkan::Buffer& buffer() const override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(x_d_);
                return x_d_->buffer();
        }

        [[nodiscard]] const vulkan::BufferWithMemory& buffer_with_memory() const override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                ASSERT(x_d_);
                return x_d_->buffer_with_memory();
        }

public:
        Impl(const vulkan::Device* const device,
             const vulkan::CommandPool* const compute_command_pool,
             const vulkan::Queue* const compute_queue,
             const vulkan::CommandPool* const transfer_command_pool,
             const vulkan::Queue* const transfer_queue,
             const vulkan::BufferMemoryType buffer_memory_type,
             const Vector2i& group_size)
                : device_(device),
                  compute_command_pool_(compute_command_pool),
                  compute_queue_(compute_queue),
                  transfer_command_pool_(transfer_command_pool),
                  transfer_queue_(transfer_queue),
                  buffer_memory_type_(buffer_memory_type),
                  group_size_(group_size),
                  mul_program_(device_->handle()),
                  mul_memory_(device_->handle(), mul_program_.descriptor_set_layout()),
                  mul_d_program_(device_->handle()),
                  mul_d_d1_fwd_(device_->handle(), mul_d_program_.descriptor_set_layout()),
                  mul_d_d1_inv_(device_->handle(), mul_d_program_.descriptor_set_layout()),
                  mul_d_d2_fwd_(device_->handle(), mul_d_program_.descriptor_set_layout()),
                  mul_d_d2_inv_(device_->handle(), mul_d_program_.descriptor_set_layout())
        {
                ASSERT(compute_command_pool->family_index() == compute_queue->family_index());
                ASSERT(transfer_command_pool->family_index() == transfer_queue->family_index());
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                device_->wait_idle_noexcept("DFT compute destructor");
        }
};
}

std::unique_ptr<Dft> create_dft(
        const vulkan::Device* const device,
        const vulkan::CommandPool* const compute_command_pool,
        const vulkan::Queue* const compute_queue,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        const vulkan::BufferMemoryType buffer_memory_type,
        const Vector2i& group_size)
{
        return std::make_unique<Impl>(
                device, compute_command_pool, compute_queue, transfer_command_pool, transfer_queue, buffer_memory_type,
                group_size);
}
}
