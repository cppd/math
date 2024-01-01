/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <complex>
#include <cstdint>
#include <vector>

namespace ns::gpu::dft
{
class ComplexNumberBuffer final
{
        unsigned size_;
        vulkan::BufferWithMemory buffer_;

public:
        ComplexNumberBuffer(
                const vulkan::Device& device,
                const std::vector<std::uint32_t>& family_indices,
                unsigned size,
                vulkan::BufferMemoryType memory_type);

        ComplexNumberBuffer(
                const vulkan::Device& device,
                const vulkan::CommandPool& transfer_command_pool,
                const vulkan::Queue& transfer_queue,
                const std::vector<std::uint32_t>& family_indices,
                const std::vector<std::complex<double>>& data);

        [[nodiscard]] unsigned size() const
        {
                return size_;
        }

        [[nodiscard]] const vulkan::Buffer& buffer() const
        {
                return buffer_.buffer();
        }

        [[nodiscard]] const vulkan::BufferWithMemory& buffer_with_memory() const
        {
                return buffer_;
        }
};
}
