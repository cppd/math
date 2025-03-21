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

#pragma once

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/layout.h>
#include <src/vulkan/objects.h>

#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
class MeshBuffer final
{
        struct Mesh final
        {
                vulkan::std140::Matrix4f model_matrix;
                vulkan::std140::Matrix3f normal_matrix;
                vulkan::std140::Vector3f color;
                float alpha;
                float ambient;
                float metalness;
                float roughness;
        };

        vulkan::BufferWithMemory uniform_buffer_;

public:
        MeshBuffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices);

        [[nodiscard]] const vulkan::Buffer& buffer() const;

        void set_coordinates(const numerical::Matrix4d& model_matrix, const numerical::Matrix3d& normal_matrix) const;
        void set_color(const numerical::Vector3f& color) const;
        void set_alpha(float alpha) const;
        void set_lighting(float ambient, float metalness, float roughness) const;
};
}
