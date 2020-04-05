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

#include <src/model/mesh.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vec.h>
#include <src/vulkan/objects.h>

#include <memory>
#include <vector>

namespace gpu
{
class MeshObject final
{
        class Triangles;
        class Lines;
        class Points;

        mat4 m_model_matrix;

        std::unique_ptr<Triangles> m_triangles;
        std::unique_ptr<Lines> m_lines;
        std::unique_ptr<Points> m_points;

public:
        MeshObject(
                const vulkan::Device& device,
                const vulkan::CommandPool& graphics_command_pool,
                const vulkan::Queue& graphics_queue,
                const vulkan::CommandPool& transfer_command_pool,
                const vulkan::Queue& transfer_queue,
                VkSampler sampler,
                VkDescriptorSetLayout descriptor_set_layout,
                const mesh::Mesh<3>& mesh,
                const mat4& model_matrix);

        ~MeshObject();

        bool has_shadow() const;
        const mat4& model_matrix() const;

        //

        struct DrawInfoAll final
        {
                struct Triangles
                {
                        VkPipelineLayout pipeline_layout;
                        VkPipeline pipeline;
                        VkDescriptorSet shared_descriptor_set;
                        uint32_t shared_descriptor_set_number;
                };

                struct Lines
                {
                        VkPipelineLayout pipeline_layout;
                        VkPipeline pipeline;
                        VkDescriptorSet descriptor_set;
                        uint32_t descriptor_set_number;
                };

                struct Points
                {
                        VkPipelineLayout pipeline_layout;
                        VkPipeline pipeline;
                        VkDescriptorSet descriptor_set;
                        uint32_t descriptor_set_number;
                };

                Triangles triangles;
                Lines lines;
                Points points;
        };

        void draw_commands_all(VkCommandBuffer buffer, const DrawInfoAll& info) const;

        //

        struct DrawInfoPlainTriangles final
        {
                VkPipelineLayout pipeline_layout;
                VkPipeline pipeline;
                VkDescriptorSet descriptor_set;
                uint32_t descriptor_set_number;
        };

        void draw_commands_plain_triangles(VkCommandBuffer buffer, const DrawInfoPlainTriangles& info) const;

        //

        struct DrawInfoTriangleVertices final
        {
                VkPipelineLayout pipeline_layout;
                VkPipeline pipeline;
                VkDescriptorSet descriptor_set;
                uint32_t descriptor_set_number;
        };

        void draw_commands_triangle_vertices(VkCommandBuffer buffer, const DrawInfoTriangleVertices& info) const;
};
}
