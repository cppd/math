/*
Copyright (C) 2017, 2018 Topological Manifold

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

#if defined(VULKAN_FOUND)

#include "objects.h"

namespace vulkan
{
#if 0
class VertexBufferWithHostVisibleMemory
{
        Buffer m_vertex_buffer;
        DeviceMemory m_vertex_device_memory;

public:
        VertexBufferWithHostVisibleMemory(const Device& device, VkDeviceSize data_size, const void* data);

        operator VkBuffer() const noexcept;
};
#endif

class VertexBufferWithDeviceLocalMemory
{
        Buffer m_vertex_buffer;
        DeviceMemory m_vertex_device_memory;

public:
        VertexBufferWithDeviceLocalMemory(const Device& device, VkCommandPool command_pool, VkQueue queue,
                                          const std::vector<uint32_t>& family_indices, VkDeviceSize data_size, const void* data);

        operator VkBuffer() const noexcept;
};
}

#endif
