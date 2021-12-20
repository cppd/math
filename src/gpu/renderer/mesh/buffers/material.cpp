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

#include "material.h"

namespace ns::gpu::renderer
{
MaterialBuffer::MaterialBuffer(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const Material& material)
        : uniform_buffer_(
                vulkan::BufferMemoryType::DEVICE_LOCAL,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                sizeof(Material))
{
        uniform_buffer_.write(command_pool, queue, data_size(material), data_pointer(material));
}

const vulkan::Buffer& MaterialBuffer::buffer() const
{
        return uniform_buffer_.buffer();
}
}
