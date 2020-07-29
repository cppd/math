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

#include <src/model/mesh_object.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>

#include <memory>

namespace gpu::renderer
{
struct MeshObject
{
        virtual ~MeshObject() = default;

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

        virtual void update(
                const std::unordered_set<mesh::Update>& updates,
                const mesh::MeshObject<3>& mesh_object,
                bool* update_command_buffers) = 0;
};

std::unique_ptr<MeshObject> create_mesh_object(
        const vulkan::Device& device,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        const std::vector<vulkan::DescriptorSetLayoutAndBindings>& mesh_layouts,
        const std::vector<vulkan::DescriptorSetLayoutAndBindings>& material_layouts,
        VkSampler texture_sampler);
}
