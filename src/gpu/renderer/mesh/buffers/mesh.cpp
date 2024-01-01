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

#include "mesh.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/layout.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
MeshBuffer::MeshBuffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices)
        : uniform_buffer_(
                vulkan::BufferMemoryType::HOST_VISIBLE,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Mesh))
{
}

const vulkan::Buffer& MeshBuffer::buffer() const
{
        return uniform_buffer_.buffer();
}

void MeshBuffer::set_coordinates(const Matrix4d& model_matrix, const Matrix3d& normal_matrix) const
{
        static_assert(offsetof(Mesh, model_matrix) + sizeof(Mesh::model_matrix) == offsetof(Mesh, normal_matrix));

        constexpr std::size_t OFFSET = offsetof(Mesh, model_matrix);
        constexpr std::size_t SIZE = offsetof(Mesh, normal_matrix) + sizeof(Mesh::normal_matrix) - OFFSET;

        const vulkan::BufferMapper map(uniform_buffer_, OFFSET, SIZE);

        const decltype(Mesh().model_matrix) model = vulkan::to_std140<float>(model_matrix);
        const decltype(Mesh().normal_matrix) normal = vulkan::to_std140<float>(normal_matrix);

        map.write(offsetof(Mesh, model_matrix) - OFFSET, model);
        map.write(offsetof(Mesh, normal_matrix) - OFFSET, normal);
}

void MeshBuffer::set_color(const Vector3f& color) const
{
        const decltype(Mesh().color) c = color;
        vulkan::map_and_write_to_buffer(uniform_buffer_, offsetof(Mesh, color), c);
}

void MeshBuffer::set_alpha(const float alpha) const
{
        const decltype(Mesh().alpha) a = alpha;
        vulkan::map_and_write_to_buffer(uniform_buffer_, offsetof(Mesh, alpha), a);
}

void MeshBuffer::set_lighting(const float ambient, const float metalness, const float roughness) const
{
        static_assert(offsetof(Mesh, roughness) - offsetof(Mesh, ambient) == 2 * sizeof(float));

        constexpr std::size_t OFFSET = offsetof(Mesh, ambient);
        constexpr std::size_t SIZE = offsetof(Mesh, roughness) + sizeof(Mesh::roughness) - OFFSET;

        const vulkan::BufferMapper map(uniform_buffer_, OFFSET, SIZE);

        Mesh mesh;
        mesh.ambient = ambient;
        mesh.metalness = metalness;
        mesh.roughness = roughness;

        map.write(0, SIZE, reinterpret_cast<const char*>(&mesh) + OFFSET);
}
}
