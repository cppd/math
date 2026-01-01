/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "barriers.h"
#include "buffer.h"
#include "function.h"

#include "shaders/bit_reverse.h"
#include "shaders/fft_global.h"
#include "shaders/fft_shared.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/group_count.h>
#include <src/com/print.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <bit>
#include <complex>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace ns::gpu::dft
{
namespace
{
constexpr unsigned GROUP_SIZE_1D = 256;

unsigned shared_size(const unsigned dft_size, const VkPhysicalDeviceLimits& limits)
{
        return dft::shared_size<std::complex<float>>(dft_size, limits.maxComputeSharedMemorySize);
}

unsigned group_size(const unsigned dft_size, const VkPhysicalDeviceLimits& limits)
{
        return dft::group_size<std::complex<float>>(
                dft_size, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                limits.maxComputeSharedMemorySize);
}

class FftShared final
{
        FftSharedProgram program_;
        FftSharedMemory memory_;
        unsigned group_count_;
        VkBuffer buffer_ = VK_NULL_HANDLE;

public:
        FftShared(
                const vulkan::Device& device,
                const unsigned n,
                const unsigned data_size,
                const unsigned n_shared,
                const bool reverse_input)
                : program_(device.handle()),
                  memory_(device.handle(), program_.descriptor_set_layout()),
                  group_count_(group_count(data_size, n_shared))
        {
                ASSERT(std::has_single_bit(n));

                const std::uint32_t n_mask = n - 1;
                const std::uint32_t n_bits = std::bit_width(n) - 1;

                program_.create_pipelines(
                        data_size, n, n_mask, n_bits, n_shared, reverse_input,
                        group_size(n, device.properties().properties_10.limits));
        }

        void set(const vulkan::Buffer& buffer)
        {
                memory_.set(buffer);
                buffer_ = buffer.handle();
        }

        void commands(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, program_.pipeline(inverse));
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, program_.pipeline_layout(),
                        FftSharedMemory::set_number(), 1, &memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, group_count_, 1, 1);

                buffer_barrier(command_buffer, buffer_);
        }
};

class BitReverse final
{
        BitReverseProgram program_;
        BitReverseMemory memory_;
        unsigned group_count_;
        VkBuffer buffer_ = VK_NULL_HANDLE;

public:
        BitReverse(const vulkan::Device& device, const unsigned n, const unsigned data_size)
                : program_(device.handle()),
                  memory_(device.handle(), program_.descriptor_set_layout()),
                  group_count_(group_count(data_size, GROUP_SIZE_1D))
        {
                ASSERT(std::has_single_bit(n));

                const std::uint32_t n_mask = n - 1;
                const std::uint32_t n_bits = std::bit_width(n) - 1;

                program_.create_pipeline(GROUP_SIZE_1D, data_size, n_mask, n_bits);
        }

        void set(const vulkan::Buffer& buffer)
        {
                memory_.set(buffer);
                buffer_ = buffer.handle();
        }

        void commands(const VkCommandBuffer command_buffer) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, program_.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, program_.pipeline_layout(),
                        BitReverseMemory::set_number(), 1, &memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, group_count_, 1, 1);

                buffer_barrier(command_buffer, buffer_);
        }
};

class FftGlobal final
{
        FftGlobalProgram program_;
        std::vector<FftGlobalBuffer> buffers_;
        std::vector<FftGlobalMemory> memories_;
        unsigned group_count_;
        VkBuffer buffer_ = VK_NULL_HANDLE;

public:
        FftGlobal(
                const vulkan::Device& device,
                const unsigned n,
                const unsigned data_size,
                const unsigned n_shared,
                const std::vector<std::uint32_t>& family_indices)
                : program_(device.handle()),
                  group_count_(group_count(data_size / 2, GROUP_SIZE_1D))
        {
                program_.create_pipelines(GROUP_SIZE_1D, data_size, n);

                unsigned m_div_2 = n_shared; // half the size of DFT
                float two_pi_div_m = PI<float> / m_div_2;
                for (; m_div_2 < n; two_pi_div_m /= 2, m_div_2 <<= 1)
                {
                        const FftGlobalBuffer& buffer = buffers_.emplace_back(device, family_indices);
                        memories_.emplace_back(device.handle(), program_.descriptor_set_layout(), buffer.buffer());
                        buffer.set(two_pi_div_m, m_div_2);
                }

                ASSERT(!memories_.empty());
                ASSERT(memories_.size() == buffers_.size());
                ASSERT(n == (n_shared << memories_.size()));
        }

        void set(const vulkan::Buffer& buffer)
        {
                for (const FftGlobalMemory& m : memories_)
                {
                        m.set(buffer);
                }
                buffer_ = buffer.handle();
        }

        void commands(const VkCommandBuffer command_buffer, const bool inverse) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, program_.pipeline(inverse));

                for (const FftGlobalMemory& m : memories_)
                {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, program_.pipeline_layout(),
                                FftGlobalMemory::set_number(), 1, &m.descriptor_set(), 0, nullptr);
                        vkCmdDispatch(command_buffer, group_count_, 1, 1);

                        buffer_barrier(command_buffer, buffer_);
                }
        }
};

class Impl final : public Fft
{
        unsigned data_size_;
        bool only_shared_;

        std::optional<FftShared> fft_shared_;
        std::optional<BitReverse> bit_reverse_;
        std::optional<FftGlobal> fft_global_;

        void set_data(const ComplexNumberBuffer& data) override
        {
                if (!fft_shared_)
                {
                        return;
                }

                if (!(data.size() >= data_size_))
                {
                        error("FFT buffer size " + to_string(data.size())
                              + " must be greater than or equal to data size " + to_string(data_size_));
                }

                fft_shared_->set(data.buffer());

                if (only_shared_)
                {
                        return;
                }

                ASSERT(bit_reverse_);
                ASSERT(fft_global_);
                bit_reverse_->set(data.buffer());
                fft_global_->set(data.buffer());
        }

        void commands(const VkCommandBuffer command_buffer, const bool inverse) const override
        {
                if (!fft_shared_)
                {
                        return;
                }

                if (only_shared_)
                {
                        fft_shared_->commands(command_buffer, inverse);
                        return;
                }

                ASSERT(bit_reverse_);
                ASSERT(fft_global_);
                // n is greater than shared size. First bit reverse
                // then compute, because calculations are in place.
                bit_reverse_->commands(command_buffer);
                fft_shared_->commands(command_buffer, inverse);
                fft_global_->commands(command_buffer, inverse);
        }

        void run_for_data(
                const bool inverse,
                const ComplexNumberBuffer& data,
                const VkDevice device,
                const VkCommandPool pool,
                const VkQueue queue) override
        {
                if (!fft_shared_)
                {
                        return;
                }

                if (!(data.size() == data_size_))
                {
                        error("FFT buffer size " + to_string(data.size()) + " must be equal to data size "
                              + to_string(data_size_));
                }

                set_data(data);

                vulkan::run_commands(
                        device, pool, queue,
                        [&](const VkCommandBuffer command_buffer)
                        {
                                commands(command_buffer, inverse);
                        });
        }

public:
        Impl(const vulkan::Device& device,
             const std::vector<std::uint32_t>& family_indices,
             const unsigned count,
             const unsigned n)
        {
                if (n == 1)
                {
                        return;
                }

                if (n <= 0)
                {
                        error("FFT size " + to_string(n) + " is not positive");
                }

                if (!std::has_single_bit(n))
                {
                        error("FFT size " + to_string(n) + " is not an integral power of 2");
                }

                const unsigned n_shared = shared_size(n, device.properties().properties_10.limits);

                data_size_ = count * n;
                only_shared_ = (n <= n_shared);

                fft_shared_.emplace(
                        device, n, data_size_, n_shared,
                        /*reverse_input=*/only_shared_);

                if (only_shared_)
                {
                        return;
                }

                bit_reverse_.emplace(device, n, data_size_);
                fft_global_.emplace(device, n, data_size_, n_shared, family_indices);
        }
};
}

std::unique_ptr<Fft> create_fft(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const unsigned count,
        const unsigned n)
{
        return std::make_unique<Impl>(device, family_indices, count, n);
}
}
