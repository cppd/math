/*
Copyright (C) 2017-2024 Topological Manifold

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
#include "render_buffers.h"
#include "shadow_mapping.h"

#include "shaders/descriptors.h"
#include "shaders/program_normals.h"
#include "shaders/program_points.h"
#include "shaders/program_triangle_lines.h"
#include "shaders/program_triangles.h"

#include <src/gpu/render_buffers.h>
#include <src/gpu/renderer/buffers/ggx_f1_albedo.h>
#include <src/gpu/renderer/buffers/opacity.h>
#include <src/gpu/renderer/code/code.h>
#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

namespace ns::gpu::renderer
{
class MeshRenderer final
{
        struct Pipelines final
        {
                vulkan::handle::Pipeline triangles;
                vulkan::handle::Pipeline triangle_lines;
                vulkan::handle::Pipeline normals;
                vulkan::handle::Pipeline points;
                vulkan::handle::Pipeline lines;
        };

        const std::thread::id thread_id_ = std::this_thread::get_id();
        const VkDevice device_;
        const bool sample_shading_;

        TrianglesProgram triangles_program_;
        SharedMemory triangles_shared_memory_;

        TriangleLinesProgram triangle_lines_program_;
        SharedMemory triangle_lines_shared_memory_;

        NormalsProgram normals_program_;
        SharedMemory normals_shared_memory_;

        PointsProgram points_program_;
        SharedMemory points_shared_memory_;

        vulkan::handle::Sampler texture_sampler_;

        std::optional<Pipelines> pipelines_opaque_;
        std::optional<Pipelines> pipelines_transparent_;

        bool has_opaque_meshes_ = false;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_all_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_transparent_as_opaque_;

        std::unique_ptr<ShadowMapping> shadow_mapping_;
        std::unique_ptr<RenderBuffers> render_buffers_;

        [[nodiscard]] const std::optional<Pipelines>& render_pipelines(bool transparent) const;
        [[nodiscard]] std::optional<Pipelines>& render_pipelines(bool transparent);

        void draw_commands(
                const std::vector<const MeshObject*>& meshes,
                VkCommandBuffer command_buffer,
                bool show_clip_plane_lines,
                bool show_normals,
                bool transparent) const;

public:
        MeshRenderer(
                const vulkan::Device* device,
                const Code& code,
                bool sample_shading,
                bool sampler_anisotropy,
                const vulkan::Buffer& drawing_buffer,
                const std::vector<std::uint32_t>& drawing_family_indices,
                const GgxF1Albedo& ggx_f1_albedo);

        [[nodiscard]] std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts() const;
        [[nodiscard]] std::vector<vulkan::DescriptorSetLayoutAndBindings> material_layouts() const;
        [[nodiscard]] VkSampler texture_sampler() const;

        void create_render_buffers(
                const RenderBuffers3D* render_buffers,
                const vulkan::ImageWithMemory& objects_image,
                const vulkan::ImageWithMemory& transparency_heads_image,
                const vulkan::ImageWithMemory& transparency_heads_size_image,
                const vulkan::Buffer& transparency_counter,
                const vulkan::Buffer& transparency_nodes,
                const Opacity& opacity,
                const numerical::Region<2, int>& viewport);

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
                bool show_clip_plane_lines,
                bool show_normals,
                const std::function<void(VkCommandBuffer command_buffer)>& before_transparency_render_pass_commands,
                const std::function<void(VkCommandBuffer command_buffer)>& after_transparency_render_pass_commands);

        void delete_render_command_buffers();

        void create_shadow_mapping_command_buffers(
                const std::vector<const MeshObject*>& meshes,
                VkCommandPool graphics_command_pool);

        void delete_shadow_mapping_command_buffers();

        void set_shadow_matrices(const numerical::Matrix4d& vp_matrix, const numerical::Matrix4d& world_to_shadow)
                const;
        void set_acceleration_structure(VkAccelerationStructureKHR acceleration_structure);

        [[nodiscard]] bool has_meshes() const;
        [[nodiscard]] bool has_opaque_meshes() const;
        [[nodiscard]] bool has_transparent_meshes() const;

        [[nodiscard]] std::optional<VkCommandBuffer> render_command_buffer_all(unsigned index) const;
        [[nodiscard]] std::optional<VkCommandBuffer> render_command_buffer_transparent_as_opaque(unsigned index) const;

        [[nodiscard]] VkCommandBuffer shadow_mapping_command_buffer(unsigned index) const;
        [[nodiscard]] const vulkan::ImageView& shadow_mapping_image_view() const;
        [[nodiscard]] VkSampler shadow_mapping_sampler() const;
};
}
