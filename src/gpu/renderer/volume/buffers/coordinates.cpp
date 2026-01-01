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

#include "coordinates.h"

#include <src/numerical/matrix.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/layout.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
VolumeCoordinatesBuffer::VolumeCoordinatesBuffer(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices)
        : buffer_(vulkan::BufferMemoryType::HOST_VISIBLE,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  sizeof(Coordinates))
{
}

const vulkan::Buffer& VolumeCoordinatesBuffer::buffer() const
{
        return buffer_.buffer();
}

void VolumeCoordinatesBuffer::set(const numerical::Matrix4d& device_to_world) const
{
        const decltype(Coordinates().device_to_world) m = vulkan::to_std140<float>(device_to_world);
        vulkan::map_and_write_to_buffer(buffer_, offsetof(Coordinates, device_to_world), m);
}

void VolumeCoordinatesBuffer::set(
        const numerical::Matrix4d& device_to_world,
        const numerical::Matrix4d& device_to_shadow,
        const numerical::Matrix4d& world_to_shadow) const
{
        Coordinates m;
        m.device_to_world = vulkan::to_std140<float>(device_to_world);
        m.device_to_shadow = vulkan::to_std140<float>(device_to_shadow);
        m.world_to_shadow = vulkan::to_std140<float>(world_to_shadow);
        vulkan::map_and_write_to_buffer(buffer_, m);
}
}
