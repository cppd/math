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

#include "../../vulkan_interfaces.h"
#include "shader/buffers.h"
#include "shader/normals.h"
#include "shader/points.h"
#include "shader/triangle_lines.h"
#include "shader/triangles.h"
#include "shader/triangles_depth.h"

#include <optional>
#include <thread>

namespace gpu
{
class MeshRenderer
{
        const std::thread::id m_thread_id = std::this_thread::get_id();
        const vulkan::Device& m_device;
        const bool m_sample_shading;

        const RenderBuffers3D* m_render_buffers = nullptr;
        const RendererDepthBuffers* m_depth_buffers = nullptr;

        RendererTrianglesProgram m_triangles_program;
        RendererTrianglesMemory m_triangles_memory;

        RendererTriangleLinesProgram m_triangle_lines_program;
        RendererTriangleLinesMemory m_triangle_lines_memory;

        RendererNormalsProgram m_normals_program;
        RendererNormalsMemory m_normals_memory;

        RendererTrianglesDepthProgram m_triangles_depth_program;
        RendererTrianglesDepthMemory m_triangles_depth_memory;

        RendererPointsProgram m_points_program;
        RendererPointsMemory m_points_memory;

        std::optional<vulkan::Pipeline> m_render_triangles_pipeline;
        std::optional<vulkan::Pipeline> m_render_triangle_lines_pipeline;
        std::optional<vulkan::Pipeline> m_render_normals_pipeline;
        std::optional<vulkan::Pipeline> m_render_points_pipeline;
        std::optional<vulkan::Pipeline> m_render_lines_pipeline;
        std::optional<vulkan::CommandBuffers> m_render_command_buffers;

        std::optional<vulkan::Pipeline> m_render_triangles_depth_pipeline;
        std::optional<vulkan::CommandBuffers> m_render_depth_command_buffers;

        vulkan::Sampler m_texture_sampler;
        vulkan::Sampler m_shadow_sampler;

        void draw_commands(
                const MeshObject* mesh,
                VkCommandBuffer command_buffer,
                bool clip_plane,
                bool normals,
                bool depth) const;

public:
        MeshRenderer(
                const vulkan::Device& device,
                bool sample_shading,
                bool sampler_anisotropy,
                const RendererBuffers& buffers);

        vulkan::Descriptors create_material_descriptors_sets(const std::vector<MaterialInfo>& materials);

        void create_render_buffers(
                const RenderBuffers3D* render_buffers,
                const vulkan::ImageWithMemory* object_image,
                const Region<2, int>& viewport);
        void delete_render_buffers();

        void create_depth_buffers(const RendererDepthBuffers* depth_buffers);
        void delete_depth_buffers();

        void create_render_command_buffers(
                const MeshObject* mesh,
                VkCommandPool graphics_command_pool,
                bool clip_plane,
                bool normals,
                const Color& clear_color,
                const std::function<void(VkCommandBuffer command_buffer)>& before_render_pass_commands);
        void delete_render_command_buffers();

        void create_depth_command_buffers(
                const MeshObject* mesh,
                VkCommandPool graphics_command_pool,
                bool clip_plane,
                bool normals);
        void delete_depth_command_buffers();

        VkCommandBuffer render_command_buffer(unsigned index) const;
        VkCommandBuffer depth_command_buffer(unsigned index) const;
};
}
