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

#pragma once

#include <src/numerical/vector.h>

#include <vulkan/vulkan_core.h>

#include <vector>

namespace ns::gpu::renderer
{
struct PointsVertex final
{
        numerical::Vector3f position;

        explicit constexpr PointsVertex(const numerical::Vector3f& position)
                : position(position)
        {
        }

        [[nodiscard]] static std::vector<VkVertexInputBindingDescription> binding_descriptions();

        [[nodiscard]] static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
};
}
