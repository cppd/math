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

#pragma once

#include <src/numerical/matrix.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/layout.h>
#include <src/vulkan/objects.h>

#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
class ShadowMatricesBuffer final
{
        struct ShadowMatrices final
        {
                vulkan::std140::Matrix4f vp_matrix;
                vulkan::std140::Matrix4f world_to_shadow;
        };

        vulkan::BufferWithMemory buffer_;

public:
        ShadowMatricesBuffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices);

        [[nodiscard]] const vulkan::Buffer& buffer() const;

        void set(const numerical::Matrix4d& vp_matrix, const numerical::Matrix4d& world_to_shadow) const;
};
}
