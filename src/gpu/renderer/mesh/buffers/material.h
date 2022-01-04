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

#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>

#include <vector>

namespace ns::gpu::renderer
{
class MaterialBuffer final
{
        vulkan::BufferWithMemory uniform_buffer_;

public:
        struct Material final
        {
                alignas(sizeof(Vector4f)) Vector3f color;
                std::uint32_t use_texture;
                std::uint32_t use_material;
        };

        MaterialBuffer(
                const vulkan::Device& device,
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                const std::vector<std::uint32_t>& family_indices,
                const Material& material);

        const vulkan::Buffer& buffer() const;
};
}