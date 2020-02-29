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

#include <src/numerical/matrix.h>
#include <src/numerical/vec.h>
#include <src/obj/obj.h>
#include <src/vulkan/objects.h>

#include <memory>
#include <vector>

namespace gpu_vulkan
{
struct DrawInfo
{
        VkPipelineLayout triangles_pipeline_layout;
        VkPipeline triangles_pipeline;
        VkDescriptorSet triangles_shared_set;
        unsigned triangles_shared_set_number;

        VkPipelineLayout points_pipeline_layout;
        VkPipeline points_pipeline;
        VkDescriptorSet points_set;
        unsigned points_set_number;

        VkPipelineLayout lines_pipeline_layout;
        VkPipeline lines_pipeline;
        VkDescriptorSet lines_set;
        unsigned lines_set_number;
};

struct DrawInfoTriangles
{
        VkPipelineLayout triangles_pipeline_layout;
        VkPipeline triangles_pipeline;
        VkDescriptorSet triangles_set;
        unsigned triangles_set_number;
};

class DrawObject final
{
        class Triangles;
        class Lines;
        class Points;

        mat4 m_model_matrix;

        std::unique_ptr<Triangles> m_triangles;
        std::unique_ptr<Lines> m_lines;
        std::unique_ptr<Points> m_points;

public:
        DrawObject(
                const vulkan::Device& device,
                const vulkan::CommandPool& graphics_command_pool,
                const vulkan::Queue& graphics_queue,
                const vulkan::CommandPool& transfer_command_pool,
                const vulkan::Queue& transfer_queue,
                VkSampler sampler,
                VkDescriptorSetLayout descriptor_set_layout,
                const Obj<3>& obj,
                double size,
                const vec3& position);
        ~DrawObject();

        bool has_shadow() const;
        const mat4& model_matrix() const;
        void draw_commands(VkCommandBuffer command_buffer, const DrawInfo& info) const;
        void draw_commands_triangles(VkCommandBuffer command_buffer, const DrawInfoTriangles& info) const;
};
}
