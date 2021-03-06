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

#pragma once

#include "function.h"

#include <src/vulkan/buffers.h>

#include <complex>
#include <unordered_set>
#include <vector>

namespace ns::gpu::dft
{
class ComplexNumberBuffer final
{
        unsigned m_size;
        vulkan::BufferWithMemory m_buffer;

public:
        ComplexNumberBuffer(
                const vulkan::Device& device,
                const std::unordered_set<uint32_t>& family_indices,
                unsigned size,
                vulkan::BufferMemoryType memory_type);

        ComplexNumberBuffer(
                const vulkan::Device& device,
                const vulkan::CommandPool& transfer_command_pool,
                const vulkan::Queue& transfer_queue,
                const std::unordered_set<uint32_t>& family_indices,
                const std::vector<std::complex<double>>& data);

        unsigned size() const
        {
                return m_size;
        }

        operator const vulkan::BufferWithMemory&() const&
        {
                return m_buffer;
        }

        operator VkBuffer() const&
        {
                return m_buffer;
        }

        operator const vulkan::BufferWithMemory&() const&& = delete;
        operator VkBuffer() const&& = delete;
};
}
