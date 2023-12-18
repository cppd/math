/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/model/mesh_object.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace ns::gpu::renderer
{
class MeshObject
{
public:
        virtual ~MeshObject() = default;

        [[nodiscard]] virtual bool transparent() const = 0;

        virtual void commands_triangles(
                VkCommandBuffer buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set,
                VkDescriptorSetLayout material_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_material_descriptor_set) const = 0;

        virtual void commands_plain_triangles(
                VkCommandBuffer buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const = 0;

        virtual void commands_triangle_vertices(
                VkCommandBuffer buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const = 0;

        virtual void commands_lines(
                VkCommandBuffer buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const = 0;

        virtual void commands_points(
                VkCommandBuffer buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const = 0;

        struct UpdateChanges final
        {
                bool matrix = false;
                bool mesh = false;
                bool transparency = false;
        };

        [[nodiscard]] virtual UpdateChanges update(const model::mesh::Reading<3>& mesh_object) = 0;

        [[nodiscard]] virtual std::optional<VkDeviceAddress> acceleration_structure_device_address() const = 0;
        [[nodiscard]] virtual const VkTransformMatrixKHR& acceleration_structure_matrix() const = 0;
};

std::unique_ptr<MeshObject> create_mesh_object(
        const vulkan::Device* device,
        bool ray_tracing,
        const std::vector<std::uint32_t>& graphics_family_indices,
        const vulkan::CommandPool* compute_command_pool,
        const vulkan::Queue* compute_queue,
        const vulkan::CommandPool* transfer_command_pool,
        const vulkan::Queue* transfer_queue,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> material_layouts,
        VkSampler texture_sampler);
}
