/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "object.h"
#include "shadow_mapping.h"

#include "../buffers/ggx_f1_albedo.h"
#include "../code/code.h"
#include "shaders/descriptors.h"
#include "shaders/program_normals.h"
#include "shaders/program_points.h"
#include "shaders/program_triangle_lines.h"
#include "shaders/program_triangles.h"

#include <src/gpu/render_buffers.h>

#include <memory>
#include <optional>
#include <thread>
#include <vector>

namespace ns::gpu::renderer
{
class MeshRenderer
{
        const std::thread::id thread_id_ = std::this_thread::get_id();
        const VkDevice device_;
        const bool sample_shading_;

        const RenderBuffers3D* render_buffers_ = nullptr;

        TrianglesProgram triangles_program_;
        SharedMemory triangles_shared_memory_;

        TriangleLinesProgram triangle_lines_program_;
        SharedMemory triangle_lines_shared_memory_;

        NormalsProgram normals_program_;
        SharedMemory normals_shared_memory_;

        PointsProgram points_program_;
        SharedMemory points_shared_memory_;

        struct Pipelines final
        {
                vulkan::handle::Pipeline triangles_fragments;
                vulkan::handle::Pipeline triangles_image;
                vulkan::handle::Pipeline triangle_lines;
                vulkan::handle::Pipeline normals;
                vulkan::handle::Pipeline points;
                vulkan::handle::Pipeline lines;
        };
        std::optional<Pipelines> pipelines_opaque_;
        std::optional<Pipelines> pipelines_transparent_;

        std::optional<vulkan::handle::CommandBuffers> command_buffers_all_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_all_image_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_transparent_as_opaque_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_transparent_as_opaque_image_;

        vulkan::handle::Sampler texture_sampler_;

        std::unique_ptr<ShadowMapping> shadow_mapping_;

        const std::optional<Pipelines>& render_pipelines(bool transparent) const;
        std::optional<Pipelines>& render_pipelines(bool transparent);

        const std::optional<vulkan::handle::CommandBuffers>& command_buffers_all(bool image) const;
        const std::optional<vulkan::handle::CommandBuffers>& command_buffers_transparent_as_opaque(bool image) const;

        void draw_commands(
                const std::vector<const MeshObject*>& meshes,
                VkCommandBuffer command_buffer,
                bool clip_plane,
                bool normals,
                bool transparent,
                bool image) const;

public:
        MeshRenderer(
                const vulkan::Device* device,
                const Code& code,
                bool sample_shading,
                bool sampler_anisotropy,
                const vulkan::Buffer& drawing_buffer,
                const std::vector<std::uint32_t>& drawing_family_indices,
                const GgxF1Albedo& ggx_f1_albedo);

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

        void create_shadow_mapping_buffers(
                unsigned buffer_count,
                const std::vector<std::uint32_t>& family_indices,
                VkCommandPool graphics_command_pool,
                VkQueue graphics_queue,
                const vulkan::Device& device,
                unsigned width,
                unsigned height,
                double zoom);
        void delete_shadow_mapping_buffers();

        void create_render_command_buffers(
                const std::vector<const MeshObject*>& meshes,
                VkCommandPool graphics_command_pool,
                bool clip_plane,
                bool normals,
                const std::function<void(VkCommandBuffer command_buffer)>& before_transparency_render_pass_commands,
                const std::function<void(VkCommandBuffer command_buffer)>& after_transparency_render_pass_commands);
        void delete_render_command_buffers();

        void create_shadow_mapping_command_buffers(
                const std::vector<const MeshObject*>& meshes,
                VkCommandPool graphics_command_pool);
        void delete_shadow_mapping_command_buffers();

        void set_shadow_matrices(const Matrix4d& vp_matrix, const Matrix4d& world_to_shadow) const;
        void set_acceleration_structure(VkAccelerationStructureKHR acceleration_structure);

        bool has_meshes() const;
        bool has_transparent_meshes() const;
        std::optional<VkCommandBuffer> render_command_buffer_all(unsigned index, bool image) const;
        std::optional<VkCommandBuffer> render_command_buffer_transparent_as_opaque(unsigned index, bool image) const;

        VkCommandBuffer shadow_mapping_command_buffer(unsigned index) const;
        const vulkan::ImageView& shadow_mapping_image_view() const;
        VkSampler shadow_mapping_sampler() const;
};
}
