/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "compute_bit_reverse.h"
#include "compute_copy_input.h"
#include "compute_copy_output.h"
#include "compute_fft_global.h"
#include "compute_fft_shared.h"
#include "compute_mul.h"
#include "compute_mul_d.h"

#include "com/bits.h"
#include "com/error.h"
#include "com/groups.h"
#include "gpu/dft/com/com.h"

#include <optional>
#include <thread>

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
};
// clang-format on

constexpr const int GROUP_SIZE_1D = 256;
constexpr const vec2i GROUP_SIZE_2D = vec2i(16, 16);

namespace gpu_vulkan
{
namespace
{
int shared_size(int dft_size, const VkPhysicalDeviceLimits& limits)
{
        return dft_shared_size<std::complex<float>>(dft_size, limits.maxComputeSharedMemorySize);
}

int group_size(int dft_size, const VkPhysicalDeviceLimits& limits)
{
        return dft_group_size<std::complex<float>>(dft_size, limits.maxComputeWorkGroupSize[0],
                                                   limits.maxComputeWorkGroupInvocations, limits.maxComputeSharedMemorySize);
}

class DeviceMemory final
{
        static constexpr VkDeviceSize COMPLEX_SIZE = 2 * sizeof(float);

        unsigned m_size;
        vulkan::BufferWithMemory m_buffer;

public:
        DeviceMemory(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices, unsigned size)
                : m_size(size),
                  m_buffer(vulkan::BufferMemoryType::DeviceLocal, device, family_indices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           size * COMPLEX_SIZE)
        {
        }

        unsigned size() const
        {
                return m_size;
        }

        operator const vulkan::BufferWithMemory&() const
        {
                return m_buffer;
        }
};

class Fft1d
{
        unsigned m_n;
        unsigned m_data_size;
        unsigned m_n_shared;
        bool m_only_shared;

        std::optional<DftFftSharedProgram> m_fft_program;
        std::optional<DftFftSharedMemory> m_fft_memory;
        int m_fft_group_count;

        std::optional<DftBitReverseProgram> m_bit_reverse_program;
        std::optional<DftBitReverseMemory> m_bit_reverse_memory;
        int m_bit_reverse_group_count;

        std::optional<DftFftGlobalProgram> m_fft_g_program;
        std::vector<DftFftGlobalMemory> m_fft_g_memory;
        int m_fft_g_group_count;

#if 0
        void commands_fft(VkCommandBuffer command_buffer, bool inverse) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fft_program->pipeline(inverse));
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fft_program->pipeline_layout(),
                                        DftFftSharedMemory::set_number(), 1, &m_fft_memory->descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_fft_group_count, 1, 1);
        }

        void commands_bit_reverse(VkCommandBuffer command_buffer) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_bit_reverse_program->pipeline());
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_bit_reverse_program->pipeline_layout(),
                                        DftBitReverseMemory::set_number(), 1, &m_bit_reverse_memory->descriptor_set(), 0,
                                        nullptr);
                vkCmdDispatch(command_buffer, m_bit_reverse_group_count, 1, 1);
        }

        void commands_fft_g(VkCommandBuffer command_buffer, bool inverse) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fft_g_program->pipeline(inverse));
                for (const DftFftGlobalMemory& m : m_fft_g_memory)
                {
                        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                                m_fft_g_program->pipeline_layout(), DftFftGlobalMemory::set_number(), 1,
                                                &m.descriptor_set(), 0, nullptr);
                        vkCmdDispatch(command_buffer, m_fft_g_group_count, 1, 1);
                }
        }
#endif

public:
        Fft1d(const vulkan::VulkanInstance& instance, const std::unordered_set<uint32_t>& family_indices, int count, int n)
        {
                if (n == 1)
                {
                        return;
                }

                m_n = n;
                m_data_size = count * n;
                m_n_shared = shared_size(n, instance.limits());
                m_only_shared = m_n <= m_n_shared;

                const uint32_t n_mask = n - 1;
                const uint32_t n_bits = binary_size(n);

                //

                const bool fft_reverse_input = m_only_shared;
                m_fft_program.emplace(instance.device());
                m_fft_program->create_pipelines(m_data_size, n, n_mask, n_bits, m_n_shared, fft_reverse_input,
                                                group_size(n, instance.limits()));
                m_fft_memory.emplace(instance.device(), m_fft_program->descriptor_set_layout());
                m_fft_group_count = group_count(m_data_size, m_n_shared);

                if (m_only_shared)
                {
                        return;
                }

                //

                m_bit_reverse_program.emplace(instance.device());
                m_bit_reverse_program->create_pipeline(GROUP_SIZE_1D, m_data_size, n_mask, n_bits);
                m_bit_reverse_memory.emplace(instance.device(), m_bit_reverse_program->descriptor_set_layout());
                m_bit_reverse_group_count = group_count(m_data_size, GROUP_SIZE_1D);

                //

                m_fft_g_program.emplace(instance.device());
                m_fft_g_program->create_pipelines(GROUP_SIZE_1D, m_data_size, n);
                m_fft_g_group_count = group_count(m_data_size / 2, GROUP_SIZE_1D);

                unsigned m_div_2 = m_n_shared; // Половина размера текущих отдельных ДПФ
                float two_pi_div_m = PI<float> / m_div_2;
                for (; m_div_2 < m_n; m_div_2 <<= 1, two_pi_div_m /= 2)
                {
                        m_fft_g_memory.emplace_back(instance.device(), m_fft_g_program->descriptor_set_layout(), family_indices);
                        m_fft_g_memory.back().set_data(two_pi_div_m, m_div_2);
                }
                ASSERT(m_fft_g_memory.size() > 0);
                ASSERT(m_n == (m_n_shared << m_fft_g_memory.size()));
        }

        void set_data(const DeviceMemory& data) const
        {
                ASSERT(data.size() >= m_data_size);
                if (m_n == 1)
                {
                        return;
                }
                m_fft_memory->set_buffer(data);
                if (m_only_shared)
                {
                        return;
                }
                m_bit_reverse_memory->set_buffer(data);
                for (const DftFftGlobalMemory& m : m_fft_g_memory)
                {
                        m.set_buffer(data);
                }
        }

#if 0
        void commands(VkCommandBuffer command_buffer, bool inverse) const
        {
                if (m_n == 1)
                {
                        return;
                }

                if (m_only_shared)
                {
                        commands_fft(command_buffer, inverse);
                        return;
                }

                // Если n превышает максимум обрабатываемых данных shared_size, то вначале
                // надо отдельно выполнить перестановку данных, а потом запускать функции
                // с отключенной перестановкой, иначе одни запуски будут вносить изменения
                // в данные других запусков, так как результат пишется в исходные данные.
                commands_bit_reverse(command_buffer);
                commands_fft(command_buffer, inverse);
                // Досчитать до нужного размера уже в глобальной памяти без разделяемой
                commands_fft_g(command_buffer, inverse);
        }
#endif
};

void image_barrier_before(VkCommandBuffer command_buffer, VkImage image)
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

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);
}

void image_barrier_after(VkCommandBuffer command_buffer, VkImage image)
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

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);
}

class Impl final : public DftCompute
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;

        DftCopyInputProgram m_copy_input_program;
        DftCopyInputMemory m_copy_input_memory;
        DftCopyOutputProgram m_copy_output_program;
        DftCopyOutputMemory m_copy_output_memory;

        std::optional<Fft1d> m_fft_n2_m1;
        std::optional<Fft1d> m_fft_n1_m2;

        int m_n1 = -1, m_n2 = -1, m_m1 = -1, m_m2 = -1;

        vec2i m_group_count_copy = vec2i(0, 0);

        std::optional<DeviceMemory> m_d1_fwd, m_d1_inv, m_d2_fwd, m_d2_inv;
        std::optional<DeviceMemory> m_x_d, m_buffer;
        VkImage m_output = VK_NULL_HANDLE;

        void compute_commands(VkCommandBuffer command_buffer) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_copy_input_program.pipeline());
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_copy_input_program.pipeline_layout(),
                                        DftCopyInputMemory::set_number(), 1, &m_copy_input_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_group_count_copy[0], m_group_count_copy[1], 1);

                //

                image_barrier_before(command_buffer, m_output);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_copy_output_program.pipeline());
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_copy_output_program.pipeline_layout(),
                                        DftCopyOutputMemory::set_number(), 1, &m_copy_output_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_group_count_copy[0], m_group_count_copy[1], 1);

                image_barrier_after(command_buffer, m_output);
        }

        void create_buffers(VkSampler sampler, const vulkan::ImageWithMemory& input, const vulkan::ImageWithMemory& output,
                            unsigned x, unsigned y, unsigned width, unsigned height, uint32_t family_index) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_output = output.image();

                //

                ASSERT(sampler != VK_NULL_HANDLE);
                ASSERT(output.width() == width && output.height() == height);
                ASSERT(width > 0 && height > 0);
                ASSERT(x + width <= input.width() && y + height <= input.height());

                m_n1 = width;
                m_n2 = height;
                m_m1 = dft_compute_m(m_n1);
                m_m2 = dft_compute_m(m_n2);
                m_group_count_copy = group_count(m_n1, m_n2, GROUP_SIZE_2D);

                const std::unordered_set<uint32_t> family_indices = {family_index};

                m_d1_fwd.emplace(m_device, family_indices, m_m1);
                m_d1_inv.emplace(m_device, family_indices, m_m1);
                m_d2_fwd.emplace(m_device, family_indices, m_m2);
                m_d2_inv.emplace(m_device, family_indices, m_m2);
                m_x_d.emplace(m_device, family_indices, m_n1 * m_n2);
                m_buffer.emplace(m_device, family_indices, std::max(m_m1 * m_n2, m_m2 * m_n1));

                m_fft_n2_m1.emplace(m_instance, family_indices, m_n2, m_m1);
                m_fft_n2_m1->set_data(*m_buffer);
                m_fft_n1_m2.emplace(m_instance, family_indices, m_n1, m_m2);
                m_fft_n1_m2->set_data(*m_buffer);

                m_copy_input_memory.set(sampler, input, *m_x_d);
                m_copy_input_program.create_pipeline(GROUP_SIZE_2D[0], GROUP_SIZE_2D[1], x, y, width, height);

                m_copy_output_memory.set(*m_x_d, output);
                m_copy_output_program.create_pipeline(GROUP_SIZE_2D[0], GROUP_SIZE_2D[1], 1.0 / (m_n1 * m_n2));
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_copy_output_program.delete_pipeline();
                m_copy_input_program.delete_pipeline();

                m_fft_n2_m1.reset();
                m_fft_n1_m2.reset();

                m_d1_fwd.reset();
                m_d1_inv.reset();
                m_d2_fwd.reset();
                m_d2_inv.reset();
                m_x_d.reset();
                m_buffer.reset();

                m_group_count_copy = vec2i(0, 0);
                m_n1 = m_n2 = m_m1 = m_m2 = -1;

                m_output = VK_NULL_HANDLE;
        }

public:
        Impl(const vulkan::VulkanInstance& instance)
                : m_instance(instance),
                  m_device(instance.device()),
                  m_copy_input_program(instance.device()),
                  m_copy_input_memory(instance.device(), m_copy_input_program.descriptor_set_layout()),
                  m_copy_output_program(instance.device()),
                  m_copy_output_memory(instance.device(), m_copy_output_program.descriptor_set_layout())
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_instance.device_wait_idle_noexcept("the Vulkan DFT compute destructor");
        }
};
}

std::vector<vulkan::PhysicalDeviceFeatures> DftCompute::required_device_features()
{
        return REQUIRED_DEVICE_FEATURES;
}

std::unique_ptr<DftCompute> create_dft_compute(const vulkan::VulkanInstance& instance)
{
        return std::make_unique<Impl>(instance);
}
}
