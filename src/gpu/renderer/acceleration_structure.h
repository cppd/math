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

#include "mesh/object.h"

#include <src/vulkan/acceleration_structure.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace ns::gpu::renderer
{
class AccelerationStructure final
{
        std::vector<std::uint32_t> family_indices_;
        std::optional<vulkan::TopLevelAccelerationStructure> acceleration_structure_;

public:
        AccelerationStructure(
                const vulkan::Device& device,
                const vulkan::CommandPool& compute_command_pool,
                const vulkan::Queue& compute_queue,
                std::vector<std::uint32_t> family_indices);

        [[nodiscard]] VkAccelerationStructureKHR handle() const;

        void create(
                const vulkan::Device& device,
                const vulkan::CommandPool& compute_command_pool,
                const vulkan::Queue& compute_queue,
                const std::vector<const MeshObject*>& meshes);

        void update_matrices(
                VkDevice device,
                const vulkan::CommandPool& compute_command_pool,
                const vulkan::Queue& compute_queue,
                const std::vector<const MeshObject*>& meshes) const;
};
}
