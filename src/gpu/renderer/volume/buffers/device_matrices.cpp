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

#include "device_matrices.h"

#include "../../../com/matrix.h"

namespace ns::gpu::renderer
{
VolumeDeviceMatricesBuffer::VolumeDeviceMatricesBuffer(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices)
        : buffer_(
                vulkan::BufferMemoryType::HOST_VISIBLE,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(DeviceMatrices))
{
}

const vulkan::Buffer& VolumeDeviceMatricesBuffer::buffer() const
{
        return buffer_.buffer();
}

void VolumeDeviceMatricesBuffer::set(const Matrix4d& device_to_world, const Matrix4d& device_to_shadow) const
{
        DeviceMatrices m;
        m.device_to_world = to_std140<float>(device_to_world);
        m.device_to_shadow = to_std140<float>(device_to_shadow);
        vulkan::map_and_write_to_buffer(buffer_, m);
}
}
