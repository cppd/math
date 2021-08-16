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

#include "depth_buffer.h"
#include "mesh_object.h"

#include "shaders/buffers.h"
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

namespace ns::gpu::renderer
{
class MeshRenderer
{
        const std::thread::id thread_id_ = std::this_thread::get_id();
        const vulkan::Device& device_;
        const bool sample_shading_;

        const RenderBuffers3D* render_buffers_ = nullptr;
        const DepthBuffers* depth_buffers_ = nullptr;

        TrianglesProgram triangles_program_;
        CommonMemory triangles_common_memory_;

        TriangleLinesProgram triangle_lines_program_;
        CommonMemory triangle_lines_common_memory_;

        NormalsProgram normals_program_;
        CommonMemory normals_common_memory_;

        TrianglesDepthProgram triangles_depth_program_;
        CommonMemory triangles_depth_common_memory_;

        PointsProgram points_program_;
        CommonMemory points_common_memory_;

        struct Pipelines
        {
                std::optional<vulkan::Pipeline> triangles;
                std::optional<vulkan::Pipeline> triangle_lines;
                std::optional<vulkan::Pipeline> normals;
                std::optional<vulkan::Pipeline> points;
                std::optional<vulkan::Pipeline> lines;
        };
        Pipelines render_pipelines_opaque_;
        Pipelines render_pipelines_transparent_;
        std::optional<vulkan::CommandBuffers> render_command_buffers_all_;
        std::optional<vulkan::CommandBuffers> render_command_buffers_transparent_as_opaque_;

        std::optional<vulkan::Pipeline> render_triangles_depth_pipeline_;
        std::optional<vulkan::CommandBuffers> render_depth_command_buffers_;

        vulkan::Sampler texture_sampler_;
        vulkan::Sampler shadow_sampler_;

        const Pipelines& render_pipelines(bool transparent) const;
        Pipelines& render_pipelines(bool transparent);

        template <template <typename...> typename T>
        void draw_commands(
                const T<const MeshObject*>& meshes,
                VkCommandBuffer command_buffer,
                bool clip_plane,
                bool normals,
                bool depth,
                bool transparent_pipeline) const;

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
                const vulkan::ImageWithMemory& transparency_heads_size_image,
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
                const std::function<void(VkCommandBuffer command_buffer)>& before_transparency_render_pass_commands,
                const std::function<void(VkCommandBuffer command_buffer)>& after_transparency_render_pass_commands);
        void delete_render_command_buffers();

        void create_depth_command_buffers(
                const std::unordered_set<const MeshObject*>& meshes,
                VkCommandPool graphics_command_pool,
                bool clip_plane,
                bool normals);
        void delete_depth_command_buffers();

        bool has_meshes() const;
        bool has_transparent_meshes() const;
        std::optional<VkCommandBuffer> render_command_buffer_all(unsigned index) const;
        std::optional<VkCommandBuffer> render_command_buffer_transparent_as_opaque(unsigned index) const;
        std::optional<VkCommandBuffer> depth_command_buffer(unsigned index) const;
};
}
