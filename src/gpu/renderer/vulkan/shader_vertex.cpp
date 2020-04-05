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

#include "shader_vertex.h"

namespace gpu
{
std::vector<VkVertexInputBindingDescription> RendererTrianglesVertex::binding_descriptions()
{
        std::vector<VkVertexInputBindingDescription> descriptions;

        {
                VkVertexInputBindingDescription d = {};
                d.binding = 0;
                d.stride = sizeof(RendererTrianglesVertex);
                d.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                descriptions.push_back(d);
        }

        return descriptions;
}

//

std::vector<VkVertexInputAttributeDescription> RendererTrianglesVertex::attribute_descriptions_triangles()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;

        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 0;
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(RendererTrianglesVertex, position);

                descriptions.push_back(d);
        }
        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 1;
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(RendererTrianglesVertex, normal);

                descriptions.push_back(d);
        }
        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 2;
                d.format = VK_FORMAT_R32G32_SFLOAT;
                d.offset = offsetof(RendererTrianglesVertex, texture_coordinates);

                descriptions.push_back(d);
        }

        return descriptions;
}

std::vector<VkVertexInputAttributeDescription> RendererTrianglesVertex::attribute_descriptions_triangle_lines()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;

        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 0;
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(RendererTrianglesVertex, position);

                descriptions.push_back(d);
        }

        return descriptions;
}

std::vector<VkVertexInputAttributeDescription> RendererTrianglesVertex::attribute_descriptions_normals()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;

        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 0;
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(RendererTrianglesVertex, position);

                descriptions.push_back(d);
        }
        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 1;
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(RendererTrianglesVertex, normal);

                descriptions.push_back(d);
        }

        return descriptions;
}

std::vector<VkVertexInputAttributeDescription> RendererTrianglesVertex::attribute_descriptions_shadow()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;

        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 0;
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(RendererTrianglesVertex, position);

                descriptions.push_back(d);
        }

        return descriptions;
}
}
