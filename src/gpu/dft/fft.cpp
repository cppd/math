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

#include "fft.h"

#include "../com/groups.h"
#include "shaders/bit_reverse.h"
#include "shaders/fft_global.h"
#include "shaders/fft_shared.h"

#include <src/com/constant.h>
#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>
#include <src/vulkan/sync.h>

#include <bit>

namespace ns::gpu::dft
{
namespace
{
constexpr const int GROUP_SIZE_1D = 256;

unsigned shared_size(const int dft_size, const VkPhysicalDeviceLimits& limits)
{
        return dft::shared_size<std::complex<float>>(dft_size, limits.maxComputeSharedMemorySize);
}

int group_size(const int dft_size, const VkPhysicalDeviceLimits& limits)
{
        return dft::group_size<std::complex<float>>(
                dft_size, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                limits.maxComputeSharedMemorySize);
}

void begin_commands(const VkCommandBuffer command_buffer)
{
        VkResult result;

        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
        }
}

void end_commands(const VkQueue queue, const VkCommandBuffer command_buffer)
{
        VkResult result;

        result = vkEndCommandBuffer(command_buffer);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkEndCommandBuffer", result);
        }

        vulkan::queue_submit(command_buffer, queue);
        vulkan::queue_wait_idle(queue);
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

class Impl final : public Fft
{
        unsigned n_;
        unsigned data_size_;
        unsigned n_shared_;
        bool only_shared_;

        std::optional<FftSharedProgram> fft_program_;
        std::optional<FftSharedMemory> fft_memory_;
        int fft_groups_;

        std::optional<BitReverseProgram> bit_reverse_program_;
        std::optional<BitReverseMemory> bit_reverse_memory_;
        int bit_reverse_groups_;

        std::optional<FftGlobalProgram> fft_g_program_;
        std::vector<FftGlobalMemory> fft_g_memory_;
        int fft_g_groups_;

        VkBuffer buffer_ = VK_NULL_HANDLE;

        void commands_fft(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft_program_->pipeline(inverse));
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft_program_->pipeline_layout(),
                        FftSharedMemory::set_number(), 1, &fft_memory_->descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, fft_groups_, 1, 1);

                buffer_barrier(command_buffer, buffer_);
        }

        void commands_bit_reverse(const VkCommandBuffer command_buffer) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, bit_reverse_program_->pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, bit_reverse_program_->pipeline_layout(),
                        BitReverseMemory::set_number(), 1, &bit_reverse_memory_->descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, bit_reverse_groups_, 1, 1);

                buffer_barrier(command_buffer, buffer_);
        }

        void commands_fft_g(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft_g_program_->pipeline(inverse));
                for (const FftGlobalMemory& m : fft_g_memory_)
                {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, fft_g_program_->pipeline_layout(),
                                FftGlobalMemory::set_number(), 1, &m.descriptor_set(), 0, nullptr);
                        vkCmdDispatch(command_buffer, fft_g_groups_, 1, 1);

                        buffer_barrier(command_buffer, buffer_);
                }
        }

        //

        void set_data(const ComplexNumberBuffer& data) override
        {
                if (n_ == 1)
                {
                        return;
                }

                ASSERT(data.size() >= data_size_);
                buffer_ = data.buffer();
                fft_memory_->set_buffer(data.buffer());
                if (only_shared_)
                {
                        return;
                }
                bit_reverse_memory_->set_buffer(data.buffer());
                for (const FftGlobalMemory& m : fft_g_memory_)
                {
                        m.set_buffer(data.buffer());
                }
        }

        void commands(const VkCommandBuffer command_buffer, const bool inverse) const override
        {
                if (n_ == 1)
                {
                        return;
                }

                if (only_shared_)
                {
                        commands_fft(command_buffer, inverse);
                        return;
                }

                // n is greater than shared_size. First bit reverse
                // then compute, because calculations are in place.
                commands_bit_reverse(command_buffer);
                commands_fft(command_buffer, inverse);
                commands_fft_g(command_buffer, inverse);
        }

        void run_for_data(
                const bool inverse,
                const ComplexNumberBuffer& data,
                const VkDevice device,
                const VkCommandPool pool,
                const VkQueue queue) override
        {
                if (n_ == 1)
                {
                        return;
                }

                ASSERT(data.size() == data_size_);

                set_data(data);

                vulkan::CommandBuffer command_buffer(device, pool);
                begin_commands(command_buffer);

                commands(command_buffer, inverse);

                end_commands(queue, command_buffer);
        }

public:
        Impl(const vulkan::Device& device,
             const std::vector<uint32_t>& family_indices,
             const unsigned count,
             const unsigned n)
                : n_(n)
        {
                if (n_ == 1)
                {
                        return;
                }

                if (n <= 0)
                {
                        error("FFT size " + std::to_string(n) + " is not positive");
                }
                if (!std::has_single_bit(n))
                {
                        error("FFT size " + std::to_string(n) + " is not an integral power of 2");
                }

                data_size_ = count * n;
                n_shared_ = shared_size(n, device.properties().properties_10.limits);
                only_shared_ = n_ <= n_shared_;

                const uint32_t n_mask = n - 1;
                const uint32_t n_bits = std::bit_width(n) - 1;

                //

                const bool fft_reverse_input = only_shared_;
                fft_program_.emplace(device);
                fft_program_->create_pipelines(
                        data_size_, n, n_mask, n_bits, n_shared_, fft_reverse_input,
                        group_size(n, device.properties().properties_10.limits));
                fft_memory_.emplace(device, fft_program_->descriptor_set_layout());
                fft_groups_ = group_count(data_size_, n_shared_);

                if (only_shared_)
                {
                        return;
                }

                //

                bit_reverse_program_.emplace(device);
                bit_reverse_program_->create_pipeline(GROUP_SIZE_1D, data_size_, n_mask, n_bits);
                bit_reverse_memory_.emplace(device, bit_reverse_program_->descriptor_set_layout());
                bit_reverse_groups_ = group_count(data_size_, GROUP_SIZE_1D);

                //

                fft_g_program_.emplace(device);
                fft_g_program_->create_pipelines(GROUP_SIZE_1D, data_size_, n);
                fft_g_groups_ = group_count(data_size_ / 2, GROUP_SIZE_1D);

                unsigned m_div_2 = n_shared_; // half the size of DFT
                float two_pi_div_m = PI<float> / m_div_2;
                for (; m_div_2 < n_; two_pi_div_m /= 2, m_div_2 <<= 1)
                {
                        fft_g_memory_.emplace_back(device, fft_g_program_->descriptor_set_layout(), family_indices);
                        fft_g_memory_.back().set_data(two_pi_div_m, m_div_2);
                }
                ASSERT(!fft_g_memory_.empty());
                ASSERT(n_ == (n_shared_ << fft_g_memory_.size()));
        }
};
}

std::unique_ptr<Fft> create_fft(
        const vulkan::Device& device,
        const std::vector<uint32_t>& family_indices,
        const unsigned count,
        const unsigned n)
{
        return std::make_unique<Impl>(device, family_indices, count, n);
}
}
