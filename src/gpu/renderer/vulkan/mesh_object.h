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

#include "shader/buffers.h"

#include <src/model/mesh_object.h>
#include <src/numerical/matrix.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace gpu::renderer
{
class MeshObject final
{
        class Triangles;
        class Lines;
        class Points;

        mat4 m_model_matrix;

        CoordinatesBuffer m_coordinates_buffer;
        std::unordered_map<VkDescriptorSetLayout, vulkan::Descriptors> m_mesh_descriptor_sets;
        MeshDescriptorSetsFunction m_mesh_descriptor_sets_function;

        std::unique_ptr<Triangles> m_triangles;
        std::unique_ptr<Lines> m_lines;
        std::unique_ptr<Points> m_points;

        void create_descriptor_sets();
        VkDescriptorSet find_mesh_descriptor_set(VkDescriptorSetLayout mesh_descriptor_set_layout) const;

public:
        MeshObject(
                const vulkan::Device& device,
                const vulkan::CommandPool& graphics_command_pool,
                const vulkan::Queue& graphics_queue,
                const vulkan::CommandPool& transfer_command_pool,
                const vulkan::Queue& transfer_queue,
                const mesh::MeshObject<3>& mesh_object,
                const MeshDescriptorSetsFunction& mesh_descriptor_sets_function,
                const MaterialDescriptorSetsFunction& material_descriptor_sets_function);

        ~MeshObject();

        //

        void commands_triangles(
                VkCommandBuffer buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set,
                VkDescriptorSetLayout material_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_material_descriptor_set) const;

        void commands_plain_triangles(
                VkCommandBuffer buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const;

        void commands_triangle_vertices(
                VkCommandBuffer buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const;

        void commands_lines(
                VkCommandBuffer buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const;

        void commands_points(
                VkCommandBuffer buffer,
                VkDescriptorSetLayout mesh_descriptor_set_layout,
                const std::function<void(VkDescriptorSet descriptor_set)>& bind_mesh_descriptor_set) const;
};
}
