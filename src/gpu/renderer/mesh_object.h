/*
Copyright (C) 2017-2021 Topological Manifold

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
#include <src/vulkan/objects.h>

#include <memory>

namespace ns::gpu::renderer
{
struct MeshObject
{
        virtual ~MeshObject() = default;

        virtual bool transparent() const = 0;

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

        struct UpdateChanges
        {
                bool command_buffers = false;
                bool transparency = false;
        };
        virtual UpdateChanges update(const mesh::Reading<3>& mesh_object) = 0;
};

std::unique_ptr<MeshObject> create_mesh_object(
        const vulkan::Device& device,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> material_layouts,
        VkSampler texture_sampler);
}
