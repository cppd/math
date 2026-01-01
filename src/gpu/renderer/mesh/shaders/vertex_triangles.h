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

#include <src/numerical/vector.h>

#include <vulkan/vulkan_core.h>

#include <vector>

namespace ns::gpu::renderer
{
struct TrianglesVertex final
{
        numerical::Vector3f position;
        numerical::Vector3f normal;
        numerical::Vector2f texture_coordinates;

        constexpr TrianglesVertex(
                const numerical::Vector3f& position,
                const numerical::Vector3f& normal,
                const numerical::Vector2f& texture_coordinates)
                : position(position),
                  normal(normal),
                  texture_coordinates(texture_coordinates)
        {
        }

        [[nodiscard]] static std::vector<VkVertexInputBindingDescription> binding_descriptions();

        [[nodiscard]] static std::vector<VkVertexInputAttributeDescription> attribute_descriptions_triangles();
        [[nodiscard]] static std::vector<VkVertexInputAttributeDescription> attribute_descriptions_shadow();
        [[nodiscard]] static std::vector<VkVertexInputAttributeDescription> attribute_descriptions_triangle_lines();
        [[nodiscard]] static std::vector<VkVertexInputAttributeDescription> attribute_descriptions_normals();
};
}
