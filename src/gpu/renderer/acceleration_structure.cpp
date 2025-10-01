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

#include "acceleration_structure.h"

#include "mesh/object.h"

#include <src/com/error.h>
#include <src/vulkan/acceleration_structure.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace ns::gpu::renderer
{
AccelerationStructure::AccelerationStructure(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        std::vector<std::uint32_t> family_indices)
        : family_indices_(std::move(family_indices))
{
        acceleration_structure_.emplace(
                vulkan::create_top_level_acceleration_structure(
                        device, compute_command_pool, compute_queue, family_indices_, {}, {}));
}

VkAccelerationStructureKHR AccelerationStructure::handle() const
{
        ASSERT(acceleration_structure_);

        return acceleration_structure_->handle();
}

void AccelerationStructure::create(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<const MeshObject*>& meshes)
{
        std::vector<std::uint64_t> bottom_level_references;
        bottom_level_references.reserve(meshes.size());

        std::vector<VkTransformMatrixKHR> bottom_level_matrices;
        bottom_level_matrices.reserve(meshes.size());

        for (const MeshObject* const mesh : meshes)
        {
                const auto address = mesh->acceleration_structure_device_address();
                if (address)
                {
                        bottom_level_references.push_back(*address);
                        bottom_level_matrices.push_back(mesh->acceleration_structure_matrix());
                }
        }

        acceleration_structure_.emplace(
                vulkan::create_top_level_acceleration_structure(
                        device, compute_command_pool, compute_queue, family_indices_, bottom_level_references,
                        bottom_level_matrices));
}

void AccelerationStructure::update_matrices(
        const VkDevice device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<const MeshObject*>& meshes) const
{
        ASSERT(acceleration_structure_);

        std::vector<VkTransformMatrixKHR> bottom_level_matrices;
        bottom_level_matrices.reserve(meshes.size());

        for (const MeshObject* const mesh : meshes)
        {
                if (mesh->acceleration_structure_device_address())
                {
                        bottom_level_matrices.push_back(mesh->acceleration_structure_matrix());
                }
        }

        acceleration_structure_->update_matrices(device, compute_command_pool, compute_queue, bottom_level_matrices);
}
}
