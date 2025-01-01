/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "shadow_matrices.h"

#include <src/numerical/matrix.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/layout.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
ShadowMatricesBuffer::ShadowMatricesBuffer(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices)
        : buffer_(vulkan::BufferMemoryType::HOST_VISIBLE,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  sizeof(ShadowMatrices))
{
}

const vulkan::Buffer& ShadowMatricesBuffer::buffer() const
{
        return buffer_.buffer();
}

void ShadowMatricesBuffer::set(const numerical::Matrix4d& vp_matrix, const numerical::Matrix4d& world_to_shadow) const
{
        ShadowMatrices shadow_matrices;
        shadow_matrices.vp_matrix = vulkan::to_std140<float>(vp_matrix);
        shadow_matrices.world_to_shadow = vulkan::to_std140<float>(world_to_shadow);
        vulkan::map_and_write_to_buffer(buffer_, shadow_matrices);
}
}
