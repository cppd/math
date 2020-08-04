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

#include "depth_buffer.h"
#include "mesh_object.h"

#include "shaders/descriptors.h"
#include "shaders/normals.h"
#include "shaders/points.h"
#include "shaders/triangle_lines.h"
#include "shaders/triangles.h"
#include "shaders/triangles_depth.h"

#include <src/gpu/buffers.h>

#include <optional>
#include <thread>
#include <unordered_set>

namespace gpu::renderer
{
class MeshRenderer
{
        const std::thread::id m_thread_id = std::this_thread::get_id();
        const vulkan::Device& m_device;
        const bool m_sample_shading;

        const RenderBuffers3D* m_render_buffers = nullptr;
        const DepthBuffers* m_depth_buffers = nullptr;

        TrianglesProgram m_triangles_program;
        CommonMemory m_triangles_common_memory;

        TriangleLinesProgram m_triangle_lines_program;
        CommonMemory m_triangle_lines_common_memory;

        NormalsProgram m_normals_program;
        CommonMemory m_normals_common_memory;

        TrianglesDepthProgram m_triangles_depth_program;
        CommonMemory m_triangles_depth_common_memory;

        PointsProgram m_points_program;
        CommonMemory m_points_common_memory;

        struct Pipelines
        {
                std::optional<vulkan::Pipeline> triangles;
                std::optional<vulkan::Pipeline> triangle_lines;
                std::optional<vulkan::Pipeline> normals;
                std::optional<vulkan::Pipeline> points;
                std::optional<vulkan::Pipeline> lines;
        };
        Pipelines m_render_pipelines_opaque;
        Pipelines m_render_pipelines_transparent;
        std::optional<vulkan::CommandBuffers> m_render_command_buffers;

        std::optional<vulkan::Pipeline> m_render_triangles_depth_pipeline;
        std::optional<vulkan::CommandBuffers> m_render_depth_command_buffers;

        vulkan::Sampler m_texture_sampler;
        vulkan::Sampler m_shadow_sampler;

        const Pipelines& render_pipelines(bool transparent) const;
        Pipelines& render_pipelines(bool transparent);

        void draw_commands(
                const std::unordered_set<const MeshObject*>& meshes,
                VkCommandBuffer command_buffer,
                bool clip_plane,
                bool normals,
                bool depth,
                bool transparent_pipeline,
                bool transparent_objects) const;

public:
        MeshRenderer(
                const vulkan::Device& device,
                bool sample_shading,
                bool sampler_anisotropy,
                const ShaderBuffers& buffers);

        std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts() const;
        std::vector<vulkan::DescriptorSetLayoutAndBindings> material_layouts() const;
        VkSampler texture_sampler() const;

        void create_render_buffers(
                const RenderBuffers3D* render_buffers,
                const vulkan::ImageWithMemory& objects_image,
                const vulkan::ImageWithMemory& transparency_heads_image,
                const vulkan::Buffer& transparency_counter,
                const vulkan::Buffer& transparency_nodes,
                const Region<2, int>& viewport);
        void delete_render_buffers();

        void create_depth_buffers(const DepthBuffers* depth_buffers);
        void delete_depth_buffers();

        void create_render_command_buffers(
                const std::unordered_set<const MeshObject*>& meshes,
                VkCommandPool graphics_command_pool,
                bool clip_plane,
                bool normals,
                const std::function<void(VkCommandBuffer command_buffer)>& before_render_pass_commands,
                const std::function<void(VkCommandBuffer command_buffer)>& after_render_pass_commands);
        void delete_render_command_buffers();

        void create_depth_command_buffers(
                const std::unordered_set<const MeshObject*>& meshes,
                VkCommandPool graphics_command_pool,
                bool clip_plane,
                bool normals);
        void delete_depth_command_buffers();

        std::optional<VkCommandBuffer> render_command_buffer(unsigned index) const;
        std::optional<VkCommandBuffer> depth_command_buffer(unsigned index) const;
};
}
