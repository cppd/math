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

#include "buffer.h"

namespace ns::gpu::dft
{
namespace
{
constexpr VkDeviceSize COMPLEX_SIZE = 2 * sizeof(float);
}

ComplexNumberBuffer::ComplexNumberBuffer(
        const vulkan::Device& device,
        const std::vector<uint32_t>& family_indices,
        unsigned size,
        vulkan::BufferMemoryType memory_type)
        : m_size(size),
          m_buffer(memory_type, device, family_indices, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size * COMPLEX_SIZE)
{
}

ComplexNumberBuffer::ComplexNumberBuffer(
        const vulkan::Device& device,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        const std::vector<uint32_t>& family_indices,
        const std::vector<std::complex<double>>& data)
        : m_size(data.size()),
          m_buffer(
                  vulkan::BufferMemoryType::DeviceLocal,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  data.size() * COMPLEX_SIZE)
{
        const std::vector<std::complex<float>>& float_data = conv<float>(data);
        m_buffer.write(transfer_command_pool, transfer_queue, data_size(float_data), data_pointer(float_data));
}
}
