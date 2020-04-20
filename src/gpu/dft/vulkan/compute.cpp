/*
Copyright (C) 2017-2020 Topological Manifold

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
По книге

Eleanor Chu, Alan George.
INSIDE the FFT BLACK BOX. Serial and Parallel Fast Fourier Transform Algorithms.
CRC Press LLC, 2000.

Chapter 13: FFTs for Arbitrary N.

В этой книге в главе 13 есть ошибки при вычислении H2

  В примере 13.4.
    Написано:
      h0, h1, h2, h3, h4, h5, 0, 0, 0, 0, 0,  0, h4, h3, h2, h1.
    Надо:
      h0, h1, h2, h3, h4, h5, 0, 0, 0, 0, 0, h5, h4, h3, h2, h1.

  В формулах 13.11, 13.23, 13.24, 13.25.
    Написано:
      h2(l) = h(l) для l = 0,...,N - 1,
      h2(l) = 0 для l = N,..., M - N + 1,
      h2(l) = h(M - l) для l = M - N + 2,..., M - 1.
    Надо:
      h2(l) = h(l) для l = 0,...,N - 1,
      h2(l) = 0 для l = N,..., M - N,
      h2(l) = h(M - l) для l = M - N + 1,..., M - 1.
*/

#include "compute.h"

#include "compute_bit_reverse.h"
#include "compute_copy_input.h"
#include "compute_copy_output.h"
#include "compute_fft_global.h"
#include "compute_fft_shared.h"
#include "compute_mul.h"
#include "compute_mul_d.h"

#include "../../com/groups.h"
#include "../com/com.h"

#include <src/com/bits.h>
#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>
#include <src/vulkan/sync.h>

#include <optional>
#include <thread>

namespace gpu::dft
{
namespace
{
// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> DFT_IMAGE_REQUIRED_DEVICE_FEATURES =
{
};
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> DFT_VECTOR_REQUIRED_DEVICE_FEATURES =
{
};
// clang-format on

constexpr const int GROUP_SIZE_1D = 256;
constexpr const vec2i GROUP_SIZE_2D = vec2i(16, 16);

int shared_size(int dft_size, const VkPhysicalDeviceLimits& limits)
{
        return dft::shared_size<std::complex<float>>(dft_size, limits.maxComputeSharedMemorySize);
}

int group_size(int dft_size, const VkPhysicalDeviceLimits& limits)
{
        return dft::group_size<std::complex<float>>(
                dft_size, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                limits.maxComputeSharedMemorySize);
}

void begin_commands(VkCommandBuffer command_buffer)
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

void end_commands(VkQueue queue, VkCommandBuffer command_buffer)
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

class DeviceMemory final
{
        static constexpr VkDeviceSize COMPLEX_SIZE = 2 * sizeof(float);

        unsigned m_size;
        vulkan::BufferWithMemory m_buffer;

public:
        DeviceMemory(
                const vulkan::Device& device,
                const std::unordered_set<uint32_t>& family_indices,
                unsigned size,
                vulkan::BufferMemoryType memory_type = vulkan::BufferMemoryType::DeviceLocal)
                : m_size(size),
                  m_buffer(memory_type, device, family_indices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size * COMPLEX_SIZE)
        {
        }

        DeviceMemory(
                const vulkan::Device& device,
                const vulkan::CommandPool& transfer_command_pool,
                const vulkan::Queue& transfer_queue,
                const std::unordered_set<uint32_t>& family_indices,
                const std::vector<std::complex<double>>& data)
                : m_size(data.size()),
                  m_buffer(
                          device,
                          transfer_command_pool,
                          transfer_queue,
                          family_indices,
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          data.size() * COMPLEX_SIZE,
                          conv<float>(data))
        {
        }

        unsigned size() const
        {
                return m_size;
        }

        operator const vulkan::BufferWithMemory &() const&
        {
                return m_buffer;
        }

        operator VkBuffer() const&
        {
                return m_buffer;
        }

        operator const vulkan::BufferWithMemory &() const&& = delete;
        operator VkBuffer() const&& = delete;
};

void buffer_barrier(VkCommandBuffer command_buffer, VkBuffer buffer)
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

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
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

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &barrier);
}

class Fft1d final
{
        unsigned m_n;
        unsigned m_data_size;
        unsigned m_n_shared;
        bool m_only_shared;

        std::optional<FftSharedProgram> m_fft_program;
        std::optional<FftSharedMemory> m_fft_memory;
        int m_fft_groups;

        std::optional<BitReverseProgram> m_bit_reverse_program;
        std::optional<BitReverseMemory> m_bit_reverse_memory;
        int m_bit_reverse_groups;

        std::optional<FftGlobalProgram> m_fft_g_program;
        std::vector<FftGlobalMemory> m_fft_g_memory;
        int m_fft_g_groups;

        VkBuffer m_buffer = VK_NULL_HANDLE;

        void commands_fft(VkCommandBuffer command_buffer, bool inverse) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fft_program->pipeline(inverse));
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fft_program->pipeline_layout(),
                        FftSharedMemory::set_number(), 1, &m_fft_memory->descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_fft_groups, 1, 1);

                buffer_barrier(command_buffer, m_buffer);
        }

        void commands_bit_reverse(VkCommandBuffer command_buffer) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_bit_reverse_program->pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_bit_reverse_program->pipeline_layout(),
                        BitReverseMemory::set_number(), 1, &m_bit_reverse_memory->descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_bit_reverse_groups, 1, 1);

                buffer_barrier(command_buffer, m_buffer);
        }

        void commands_fft_g(VkCommandBuffer command_buffer, bool inverse) const
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fft_g_program->pipeline(inverse));
                for (const FftGlobalMemory& m : m_fft_g_memory)
                {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fft_g_program->pipeline_layout(),
                                FftGlobalMemory::set_number(), 1, &m.descriptor_set(), 0, nullptr);
                        vkCmdDispatch(command_buffer, m_fft_g_groups, 1, 1);

                        buffer_barrier(command_buffer, m_buffer);
                }
        }

public:
        Fft1d(const vulkan::VulkanInstance& instance,
              const std::unordered_set<uint32_t>& family_indices,
              int count,
              int n)
                : m_n(n)
        {
                if (m_n == 1)
                {
                        return;
                }

                m_data_size = count * n;
                m_n_shared = shared_size(n, instance.limits());
                m_only_shared = m_n <= m_n_shared;

                const uint32_t n_mask = n - 1;
                const uint32_t n_bits = binary_size(n);

                //

                const bool fft_reverse_input = m_only_shared;
                m_fft_program.emplace(instance.device());
                m_fft_program->create_pipelines(
                        m_data_size, n, n_mask, n_bits, m_n_shared, fft_reverse_input,
                        group_size(n, instance.limits()));
                m_fft_memory.emplace(instance.device(), m_fft_program->descriptor_set_layout());
                m_fft_groups = group_count(m_data_size, m_n_shared);

                if (m_only_shared)
                {
                        return;
                }

                //

                m_bit_reverse_program.emplace(instance.device());
                m_bit_reverse_program->create_pipeline(GROUP_SIZE_1D, m_data_size, n_mask, n_bits);
                m_bit_reverse_memory.emplace(instance.device(), m_bit_reverse_program->descriptor_set_layout());
                m_bit_reverse_groups = group_count(m_data_size, GROUP_SIZE_1D);

                //

                m_fft_g_program.emplace(instance.device());
                m_fft_g_program->create_pipelines(GROUP_SIZE_1D, m_data_size, n);
                m_fft_g_groups = group_count(m_data_size / 2, GROUP_SIZE_1D);

                unsigned m_div_2 = m_n_shared; // Половина размера текущих отдельных ДПФ
                float two_pi_div_m = PI<float> / m_div_2;
                for (; m_div_2 < m_n; two_pi_div_m /= 2, m_div_2 <<= 1)
                {
                        m_fft_g_memory.emplace_back(
                                instance.device(), m_fft_g_program->descriptor_set_layout(), family_indices);
                        m_fft_g_memory.back().set_data(two_pi_div_m, m_div_2);
                }
                ASSERT(!m_fft_g_memory.empty());
                ASSERT(m_n == (m_n_shared << m_fft_g_memory.size()));
        }

        void set_data(const DeviceMemory& data)
        {
                if (m_n == 1)
                {
                        return;
                }

                ASSERT(data.size() >= m_data_size);
                m_buffer = data;
                m_fft_memory->set_buffer(data);
                if (m_only_shared)
                {
                        return;
                }
                m_bit_reverse_memory->set_buffer(data);
                for (const FftGlobalMemory& m : m_fft_g_memory)
                {
                        m.set_buffer(data);
                }
        }

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

        void run_for_data(bool inverse, const DeviceMemory& data, VkDevice device, VkCommandPool pool, VkQueue queue)
        {
                if (m_n == 1)
                {
                        return;
                }

                ASSERT(data.size() == m_data_size);

                set_data(data);

                vulkan::CommandBuffer command_buffer(device, pool);
                begin_commands(command_buffer);

                commands(command_buffer, inverse);

                end_commands(queue, command_buffer);
        }
};

class Dft final
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;

        const vulkan::CommandPool& m_compute_command_pool;
        const vulkan::Queue& m_compute_queue;
        const vulkan::CommandPool& m_transfer_command_pool;
        const vulkan::Queue& m_transfer_queue;

        const vulkan::BufferMemoryType m_buffer_memory_type;

        MulProgram m_mul_program;
        MulMemory m_mul_memory;
        vec2i m_mul_rows_to_buffer_groups = vec2i(0, 0);
        vec2i m_mul_rows_from_buffer_groups = vec2i(0, 0);
        vec2i m_mul_columns_to_buffer_groups = vec2i(0, 0);
        vec2i m_mul_columns_from_buffer_groups = vec2i(0, 0);

        MulDProgram m_mul_d_program;
        MulDMemory m_mul_d_d1_fwd;
        MulDMemory m_mul_d_d1_inv;
        MulDMemory m_mul_d_d2_fwd;
        MulDMemory m_mul_d_d2_inv;
        vec2i m_mul_d_row_groups = vec2i(0, 0);
        vec2i m_mul_d_column_groups = vec2i(0, 0);

        std::optional<Fft1d> m_fft_n2_m1;
        std::optional<Fft1d> m_fft_n1_m2;

        int m_n1 = -1;
        int m_n2 = -1;
        int m_m1 = -1;
        int m_m2 = -1;

        std::optional<DeviceMemory> m_d1_fwd;
        std::optional<DeviceMemory> m_d1_inv;
        std::optional<DeviceMemory> m_d2_fwd;
        std::optional<DeviceMemory> m_d2_inv;
        std::optional<DeviceMemory> m_x_d;
        std::optional<DeviceMemory> m_buffer;

        void rows_to_buffer(VkCommandBuffer command_buffer, bool inverse) const
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_mul_program.pipeline_rows_to_buffer(inverse));
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_mul_program.pipeline_layout(),
                        MulMemory::set_number(), 1, &m_mul_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_mul_rows_to_buffer_groups[0], m_mul_rows_to_buffer_groups[1], 1);

                buffer_barrier(command_buffer, *m_buffer);
        }

        void rows_mul_d(VkCommandBuffer command_buffer, bool inverse) const
        {
                const VkDescriptorSet* set =
                        inverse ? &m_mul_d_d1_inv.descriptor_set() : &m_mul_d_d1_fwd.descriptor_set();
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_mul_d_program.pipeline_rows());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_mul_d_program.pipeline_layout(),
                        MulDMemory::set_number(), 1, set, 0, nullptr);
                vkCmdDispatch(command_buffer, m_mul_d_row_groups[0], m_mul_d_row_groups[1], 1);

                buffer_barrier(command_buffer, *m_buffer);
        }

        void rows_from_buffer(VkCommandBuffer command_buffer, bool inverse) const
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                        m_mul_program.pipeline_rows_from_buffer(inverse));
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_mul_program.pipeline_layout(),
                        MulMemory::set_number(), 1, &m_mul_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_mul_rows_from_buffer_groups[0], m_mul_rows_from_buffer_groups[1], 1);

                buffer_barrier(command_buffer, *m_x_d);
        }

        void columns_to_buffer(VkCommandBuffer command_buffer, bool inverse) const
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                        m_mul_program.pipeline_columns_to_buffer(inverse));
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_mul_program.pipeline_layout(),
                        MulMemory::set_number(), 1, &m_mul_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_mul_columns_to_buffer_groups[0], m_mul_columns_to_buffer_groups[1], 1);

                buffer_barrier(command_buffer, *m_buffer);
        }

        void columns_mul_d(VkCommandBuffer command_buffer, bool inverse) const
        {
                const VkDescriptorSet* set =
                        inverse ? &m_mul_d_d2_inv.descriptor_set() : &m_mul_d_d2_fwd.descriptor_set();
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_mul_d_program.pipeline_columns());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_mul_d_program.pipeline_layout(),
                        MulDMemory::set_number(), 1, set, 0, nullptr);
                vkCmdDispatch(command_buffer, m_mul_d_column_groups[0], m_mul_d_column_groups[1], 1);

                buffer_barrier(command_buffer, *m_buffer);
        }

        void columns_from_buffer(VkCommandBuffer command_buffer, bool inverse) const
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                        m_mul_program.pipeline_columns_from_buffer(inverse));
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_mul_program.pipeline_layout(),
                        MulMemory::set_number(), 1, &m_mul_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(
                        command_buffer, m_mul_columns_from_buffer_groups[0], m_mul_columns_from_buffer_groups[1], 1);

                buffer_barrier(command_buffer, *m_x_d);
        }

        void create_diagonals(uint32_t family_index)
        {
                // Compute the diagonal D in Lemma 13.2: use the radix-2 FFT
                // Формулы 13.13, 13.26.

                // Для обратного преобразования нужна корректировка данных с умножением на коэффициент,
                // так как разный размер у исходного вектора N и его расширенного M.
                const double m1_div_n1 = static_cast<double>(m_m1) / m_n1;
                const double m2_div_n2 = static_cast<double>(m_m2) / m_n2;

                std::vector<std::complex<double>> d1_fwd = compute_h2(m_n1, m_m1, compute_h(m_n1, false, 1.0));
                std::vector<std::complex<double>> d1_inv = compute_h2(m_n1, m_m1, compute_h(m_n1, true, m1_div_n1));
                std::vector<std::complex<double>> d2_fwd = compute_h2(m_n2, m_m2, compute_h(m_n2, false, 1.0));
                std::vector<std::complex<double>> d2_inv = compute_h2(m_n2, m_m2, compute_h(m_n2, true, m2_div_n2));

                //

                const std::unordered_set<uint32_t> family_indices = {
                        family_index, m_compute_command_pool.family_index(), m_transfer_command_pool.family_index()};

                m_d1_fwd.emplace(m_device, m_transfer_command_pool, m_transfer_queue, family_indices, d1_fwd);
                m_d1_inv.emplace(m_device, m_transfer_command_pool, m_transfer_queue, family_indices, d1_inv);
                m_d2_fwd.emplace(m_device, m_transfer_command_pool, m_transfer_queue, family_indices, d2_fwd);
                m_d2_inv.emplace(m_device, m_transfer_command_pool, m_transfer_queue, family_indices, d2_inv);

                {
                        Fft1d fft(m_instance, {m_compute_command_pool.family_index()}, 1, m_m1);
                        fft.run_for_data(false, *m_d1_fwd, m_device, m_compute_command_pool, m_compute_queue);
                        fft.run_for_data(true, *m_d1_inv, m_device, m_compute_command_pool, m_compute_queue);
                }
                {
                        Fft1d fft(m_instance, {m_compute_command_pool.family_index()}, 1, m_m2);
                        fft.run_for_data(false, *m_d2_fwd, m_device, m_compute_command_pool, m_compute_queue);
                        fft.run_for_data(true, *m_d2_inv, m_device, m_compute_command_pool, m_compute_queue);
                }
        }

public:
        void create_buffers(unsigned width, unsigned height, uint32_t family_index)
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(width > 0 && height > 0);

                m_n1 = width;
                m_n2 = height;
                m_m1 = compute_m(m_n1);
                m_m2 = compute_m(m_n2);

                create_diagonals(family_index);

                const std::unordered_set<uint32_t> family_indices = {family_index};

                m_x_d.emplace(m_device, family_indices, m_n1 * m_n2, m_buffer_memory_type);
                m_buffer.emplace(m_device, family_indices, std::max(m_m1 * m_n2, m_m2 * m_n1));

                m_fft_n2_m1.emplace(m_instance, family_indices, m_n2, m_m1);
                m_fft_n2_m1->set_data(*m_buffer);
                m_fft_n1_m2.emplace(m_instance, family_indices, m_n1, m_m2);
                m_fft_n1_m2->set_data(*m_buffer);

                m_mul_memory.set(*m_x_d, *m_buffer);
                m_mul_program.create_pipelines(m_n1, m_n2, m_m1, m_m2, GROUP_SIZE_2D[0], GROUP_SIZE_2D[1]);
                m_mul_rows_to_buffer_groups = group_count(m_m1, m_n2, GROUP_SIZE_2D);
                m_mul_rows_from_buffer_groups = group_count(m_n1, m_n2, GROUP_SIZE_2D);
                m_mul_columns_to_buffer_groups = group_count(m_n1, m_m2, GROUP_SIZE_2D);
                m_mul_columns_from_buffer_groups = group_count(m_n1, m_n2, GROUP_SIZE_2D);

                m_mul_d_d1_fwd.set(*m_d1_fwd, *m_buffer);
                m_mul_d_d1_inv.set(*m_d1_inv, *m_buffer);
                m_mul_d_d2_fwd.set(*m_d2_fwd, *m_buffer);
                m_mul_d_d2_inv.set(*m_d2_inv, *m_buffer);
                m_mul_d_program.create_pipelines(m_n1, m_n2, m_m1, m_m2, GROUP_SIZE_2D[0], GROUP_SIZE_2D[1]);
                m_mul_d_row_groups = group_count(m_m1, m_n2, GROUP_SIZE_2D);
                m_mul_d_column_groups = group_count(m_m2, m_n1, GROUP_SIZE_2D);
        }

        void delete_buffers()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_mul_program.delete_pipelines();
                m_mul_d_program.delete_pipelines();

                m_fft_n2_m1.reset();
                m_fft_n1_m2.reset();

                m_d1_fwd.reset();
                m_d1_inv.reset();
                m_d2_fwd.reset();
                m_d2_inv.reset();
                m_x_d.reset();
                m_buffer.reset();

                m_n1 = m_n2 = m_m1 = m_m2 = -1;
        }

        void compute_commands(VkCommandBuffer command_buffer, bool inverse) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                if (m_n1 > 1)
                {
                        rows_to_buffer(command_buffer, inverse);
                        m_fft_n2_m1->commands(command_buffer, inverse);
                        rows_mul_d(command_buffer, inverse);
                        m_fft_n2_m1->commands(command_buffer, !inverse);
                        rows_from_buffer(command_buffer, inverse);
                }
                if (m_n2 > 1)
                {
                        columns_to_buffer(command_buffer, inverse);
                        m_fft_n1_m2->commands(command_buffer, inverse);
                        columns_mul_d(command_buffer, inverse);
                        m_fft_n1_m2->commands(command_buffer, !inverse);
                        columns_from_buffer(command_buffer, inverse);
                }
        }

        const vulkan::BufferWithMemory& buffer() const
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(m_x_d);
                return *m_x_d;
        }

        Dft(const vulkan::VulkanInstance& instance,
            const vulkan::CommandPool& compute_command_pool,
            const vulkan::Queue& compute_queue,
            const vulkan::CommandPool& transfer_command_pool,
            const vulkan::Queue& transfer_queue,
            vulkan::BufferMemoryType buffer_memory_type)
                : m_instance(instance),
                  m_device(instance.device()),
                  m_compute_command_pool(compute_command_pool),
                  m_compute_queue(compute_queue),
                  m_transfer_command_pool(transfer_command_pool),
                  m_transfer_queue(transfer_queue),
                  m_buffer_memory_type(buffer_memory_type),
                  m_mul_program(instance.device()),
                  m_mul_memory(instance.device(), m_mul_program.descriptor_set_layout()),
                  m_mul_d_program(instance.device()),
                  m_mul_d_d1_fwd(instance.device(), m_mul_d_program.descriptor_set_layout()),
                  m_mul_d_d1_inv(instance.device(), m_mul_d_program.descriptor_set_layout()),
                  m_mul_d_d2_fwd(instance.device(), m_mul_d_program.descriptor_set_layout()),
                  m_mul_d_d2_inv(instance.device(), m_mul_d_program.descriptor_set_layout())
        {
                ASSERT(compute_command_pool.family_index() == compute_queue.family_index());
                ASSERT(transfer_command_pool.family_index() == transfer_queue.family_index());
        }

        ~Dft()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_instance.device_wait_idle_noexcept("the Vulkan DFT compute destructor");
        }
};

class DftImage final : public ComputeImage
{
        Dft m_dft;

        CopyInputProgram m_copy_input_program;
        CopyInputMemory m_copy_input_memory;
        CopyOutputProgram m_copy_output_program;
        CopyOutputMemory m_copy_output_memory;
        vec2i m_copy_groups = vec2i(0, 0);

        VkImage m_output = VK_NULL_HANDLE;

        void create_buffers(
                VkSampler sampler,
                const vulkan::ImageWithMemory& input,
                const vulkan::ImageWithMemory& output,
                const Region<2, int>& rectangle,
                uint32_t family_index) override
        {
                ASSERT(sampler != VK_NULL_HANDLE);

                ASSERT(rectangle.width() == static_cast<int>(output.width()));
                ASSERT(rectangle.height() == static_cast<int>(output.height()));
                ASSERT(rectangle.x1() <= static_cast<int>(input.width()));
                ASSERT(rectangle.y1() <= static_cast<int>(input.height()));

                m_dft.create_buffers(rectangle.width(), rectangle.height(), family_index);

                //

                m_copy_input_memory.set(sampler, input, m_dft.buffer());
                m_copy_input_program.create_pipeline(GROUP_SIZE_2D[0], GROUP_SIZE_2D[1], rectangle);

                const int width = rectangle.width();
                const int height = rectangle.height();

                m_copy_output_memory.set(m_dft.buffer(), output);
                m_copy_output_program.create_pipeline(GROUP_SIZE_2D[0], GROUP_SIZE_2D[1], 1.0 / (width * height));

                m_copy_groups = group_count(width, height, GROUP_SIZE_2D);

                m_output = output.image();
        }

        void delete_buffers() override
        {
                m_output = VK_NULL_HANDLE;

                m_copy_output_program.delete_pipeline();
                m_copy_input_program.delete_pipeline();

                //

                m_dft.delete_buffers();
        }

        void compute_commands(VkCommandBuffer command_buffer) const override
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_copy_input_program.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_copy_input_program.pipeline_layout(),
                        CopyInputMemory::set_number(), 1, &m_copy_input_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_copy_groups[0], m_copy_groups[1], 1);

                buffer_barrier(command_buffer, m_dft.buffer());

                //

                constexpr bool inverse = false;
                m_dft.compute_commands(command_buffer, inverse);

                //

                image_barrier_before(command_buffer, m_output);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_copy_output_program.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_copy_output_program.pipeline_layout(),
                        CopyOutputMemory::set_number(), 1, &m_copy_output_memory.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, m_copy_groups[0], m_copy_groups[1], 1);

                image_barrier_after(command_buffer, m_output);
        }

public:
        DftImage(
                const vulkan::VulkanInstance& instance,
                const vulkan::CommandPool& compute_command_pool,
                const vulkan::Queue& compute_queue,
                const vulkan::CommandPool& transfer_command_pool,
                const vulkan::Queue& transfer_queue)
                : m_dft(instance,
                        compute_command_pool,
                        compute_queue,
                        transfer_command_pool,
                        transfer_queue,
                        vulkan::BufferMemoryType::DeviceLocal),
                  m_copy_input_program(instance.device()),
                  m_copy_input_memory(instance.device(), m_copy_input_program.descriptor_set_layout()),
                  m_copy_output_program(instance.device()),
                  m_copy_output_memory(instance.device(), m_copy_output_program.descriptor_set_layout())
        {
        }
};

class DftVector final : public ComputeVector
{
        vulkan::VulkanInstance m_instance;
        const vulkan::Device& m_device;

        const vulkan::CommandPool& m_compute_command_pool;
        const vulkan::Queue& m_compute_queue;

        Dft m_dft;

        std::optional<vulkan::CommandBuffers> m_command_buffers;
        unsigned m_width = 0;
        unsigned m_height = 0;

        enum DftType
        {
                Forward,
                Inverse
        };

        void delete_buffers()
        {
                m_width = -1;
                m_height = -1;

                m_command_buffers.reset();
                m_dft.delete_buffers();
        }

        void create_buffers(unsigned width, unsigned height) override
        {
                delete_buffers();

                //

                m_dft.create_buffers(width, height, m_compute_queue.family_index());

                m_command_buffers = vulkan::CommandBuffers(m_device, m_compute_command_pool, 2);
                VkResult result;
                for (int index : {DftType::Forward, DftType::Inverse})
                {
                        VkCommandBuffer command_buffer = (*m_command_buffers)[index];

                        VkCommandBufferBeginInfo command_buffer_info = {};
                        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                        result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
                        if (result != VK_SUCCESS)
                        {
                                vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
                        }

                        //

                        const bool inverse = (index == DftType::Inverse);
                        m_dft.compute_commands(command_buffer, inverse);

                        //

                        result = vkEndCommandBuffer(command_buffer);
                        if (result != VK_SUCCESS)
                        {
                                vulkan::vulkan_function_error("vkEndCommandBuffer", result);
                        }
                }

                m_width = width;
                m_height = height;
        }

        void exec(bool inverse, std::vector<std::complex<float>>* src) override
        {
                if (!(m_width > 0 && m_height > 0 && m_command_buffers))
                {
                        error("No DFT buffers");
                }
                if (!(src && (src->size() == static_cast<size_t>(m_width) * m_height)))
                {
                        error("Wrong DFT buffer size");
                }

                {
                        vulkan::BufferMapper mapper(m_dft.buffer());
                        mapper.write(*src);
                }
                vulkan::queue_submit(
                        (*m_command_buffers)[inverse ? DftType::Inverse : DftType::Forward], m_compute_queue);
                vulkan::queue_wait_idle(m_compute_queue);
                {
                        vulkan::BufferMapper mapper(m_dft.buffer());
                        mapper.read(src);
                }
        }

public:
        DftVector()
                : m_instance({}, {}, DFT_VECTOR_REQUIRED_DEVICE_FEATURES, {}),
                  m_device(m_instance.device()),
                  m_compute_command_pool(m_instance.compute_command_pool()),
                  m_compute_queue(m_instance.compute_queue()),
                  m_dft(m_instance,
                        m_compute_command_pool,
                        m_compute_queue,
                        m_instance.transfer_command_pool(),
                        m_instance.transfer_queue(),
                        vulkan::BufferMemoryType::HostVisible)
        {
        }
};
}

std::vector<vulkan::PhysicalDeviceFeatures> ComputeImage::required_device_features()
{
        return DFT_IMAGE_REQUIRED_DEVICE_FEATURES;
}

std::unique_ptr<ComputeImage> create_compute_image(
        const vulkan::VulkanInstance& instance,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue)
{
        return std::make_unique<DftImage>(
                instance, compute_command_pool, compute_queue, transfer_command_pool, transfer_queue);
}

std::unique_ptr<ComputeVector> create_compute_vector()
{
        return std::make_unique<DftVector>();
}
}
