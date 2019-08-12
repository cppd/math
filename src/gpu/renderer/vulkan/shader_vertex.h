/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/vec.h"

#include <vector>
#include <vulkan/vulkan.h>

namespace gpu_vulkan
{
struct RendererVertex
{
        vec3f position;
        vec3f normal;
        vec2f texture_coordinates;

        constexpr RendererVertex(const vec3f& position_, const vec3f& normal_, const vec2f& texture_coordinates_)
                : position(position_), normal(normal_), texture_coordinates(texture_coordinates_)
        {
        }

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();

        static std::vector<VkVertexInputAttributeDescription> triangles_attribute_descriptions();

        static std::vector<VkVertexInputAttributeDescription> shadow_attribute_descriptions();
};

struct RendererPointVertex
{
        vec3f position;

        constexpr RendererPointVertex(const vec3f& position_) : position(position_)
        {
        }

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();

        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
};
}
