/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <src/numerical/vec.h>

#include <vector>
#include <vulkan/vulkan.h>

namespace gpu::renderer
{
struct TrianglesVertex
{
        vec3f position;
        vec3f normal;
        vec2f texture_coordinates;

        constexpr TrianglesVertex(const vec3f& position, const vec3f& normal, const vec2f& texture_coordinates)
                : position(position), normal(normal), texture_coordinates(texture_coordinates)
        {
        }

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();

        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions_triangles();
        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions_triangles_depth();
        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions_triangle_lines();
        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions_normals();
};
}
