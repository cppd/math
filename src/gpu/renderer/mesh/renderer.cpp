/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "renderer.h"

#include "commands.h"
#include "meshes.h"
#include "object.h"
#include "render_buffers.h"
#include "sampler.h"
#include "shadow_mapping.h"

#include "shaders/descriptors.h"
#include "shaders/program_normals.h"
#include "shaders/program_points.h"
#include "shaders/program_triangle_lines.h"
#include "shaders/program_triangles.h"

#include <src/com/error.h>
#include <src/gpu/render_buffers.h>
#include <src/gpu/renderer/buffers/ggx_f1_albedo.h>
#include <src/gpu/renderer/buffers/opacity.h>
#include <src/gpu/renderer/code/code.h>
#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/commands.h>
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
MeshRenderer::MeshRenderer(
        const vulkan::Device* const device,
        const Code& code,
        const bool sample_shading,
        const bool sampler_anisotropy,
        const vulkan::Buffer& drawing_buffer,
        const std::vector<std::uint32_t>& drawing_family_indices,
        const GgxF1Albedo& ggx_f1_albedo)
        : device_(device->handle()),
          sample_shading_(sample_shading),
          //
          triangles_program_(device, code),
          triangles_shared_memory_(
                  device_,
                  triangles_program_.descriptor_set_layout_shared(),
                  triangles_program_.descriptor_set_layout_shared_bindings(),
                  drawing_buffer),
          //
          triangle_lines_program_(device, code),
          triangle_lines_shared_memory_(
                  device_,
                  triangle_lines_program_.descriptor_set_layout_shared(),
                  triangle_lines_program_.descriptor_set_layout_shared_bindings(),
                  drawing_buffer),
          //
          normals_program_(device, code),
          normals_shared_memory_(
                  device_,
                  normals_program_.descriptor_set_layout_shared(),
                  normals_program_.descriptor_set_layout_shared_bindings(),
                  drawing_buffer),
          //
          //
          points_program_(device, code),
          points_shared_memory_(
                  device_,
                  points_program_.descriptor_set_layout_shared(),
                  points_program_.descriptor_set_layout_shared_bindings(),
                  drawing_buffer),
          //
          texture_sampler_(create_mesh_texture_sampler(*device, sampler_anisotropy))
{
        triangles_shared_memory_.set_ggx_f1_albedo(
                ggx_f1_albedo.sampler(), ggx_f1_albedo.cosine_roughness(), ggx_f1_albedo.cosine_weighted_average());

        if (!code.ray_tracing())
        {
                shadow_mapping_ = std::make_unique<ShadowMapping>(device, code, drawing_buffer, drawing_family_indices);
                triangles_shared_memory_.set_shadow_matrices(shadow_mapping_->shadow_matrices_buffer());
        }
}

const std::optional<MeshRenderer::Pipelines>& MeshRenderer::render_pipelines(const bool transparent) const
{
        if (transparent)
        {
                return pipelines_transparent_;
        }
        return pipelines_opaque_;
}

std::optional<MeshRenderer::Pipelines>& MeshRenderer::render_pipelines(const bool transparent)
{
        if (transparent)
        {
                return pipelines_transparent_;
        }
        return pipelines_opaque_;
}

void MeshRenderer::create_render_buffers(
        const RenderBuffers3D* const render_buffers,
        const vulkan::ImageWithMemory& objects_image,
        const vulkan::ImageWithMemory& transparency_heads_image,
        const vulkan::ImageWithMemory& transparency_heads_size_image,
        const vulkan::Buffer& transparency_counter,
        const vulkan::Buffer& transparency_nodes,
        const Opacity& opacity,
        const numerical::Region<2, int>& viewport)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        delete_render_buffers();

        render_buffers_ = renderer::create_render_buffers(render_buffers, opacity, device_);

        const vulkan::RenderPass& render_pass = render_buffers_->render_pass();
        const VkSampleCountFlagBits sample_count = render_buffers_->sample_count();

        triangles_shared_memory_.set_objects_image(objects_image.image_view());
        triangles_shared_memory_.set_transparency(
                transparency_heads_image.image_view(), transparency_heads_size_image.image_view(), transparency_counter,
                transparency_nodes);

        triangle_lines_shared_memory_.set_objects_image(objects_image.image_view());
        triangle_lines_shared_memory_.set_transparency(
                transparency_heads_image.image_view(), transparency_heads_size_image.image_view(), transparency_counter,
                transparency_nodes);

        points_shared_memory_.set_objects_image(objects_image.image_view());
        points_shared_memory_.set_transparency(
                transparency_heads_image.image_view(), transparency_heads_size_image.image_view(), transparency_counter,
                transparency_nodes);

        normals_shared_memory_.set_objects_image(objects_image.image_view());
        normals_shared_memory_.set_transparency(
                transparency_heads_image.image_view(), transparency_heads_size_image.image_view(), transparency_counter,
                transparency_nodes);

        for (const bool transparent : {false, true})
        {
                Pipelines& pipelines = render_pipelines(transparent).emplace();

                pipelines.triangles = triangles_program_.create_pipeline(
                        render_pass, sample_count, sample_shading_, viewport, transparent);

                pipelines.triangle_lines = triangle_lines_program_.create_pipeline(
                        render_pass, sample_count, sample_shading_, viewport, transparent);

                pipelines.normals = normals_program_.create_pipeline(
                        render_pass, sample_count, sample_shading_, viewport, transparent);

                pipelines.points = points_program_.create_pipeline(
                        render_pass, sample_count, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, viewport, transparent);

                pipelines.lines = points_program_.create_pipeline(
                        render_pass, sample_count, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, viewport, transparent);
        }
}

void MeshRenderer::delete_render_buffers()
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        delete_render_command_buffers();

        for (const bool transparent : {false, true})
        {
                render_pipelines(transparent).reset();
        }

        render_buffers_.reset();
}

void MeshRenderer::create_shadow_mapping_buffers(
        const unsigned buffer_count,
        const std::vector<std::uint32_t>& family_indices,
        const VkCommandPool graphics_command_pool,
        const VkQueue graphics_queue,
        const vulkan::Device& device,
        const unsigned width,
        const unsigned height,
        const double zoom)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        ASSERT(shadow_mapping_);
        shadow_mapping_->create_buffers(
                buffer_count, family_indices, graphics_command_pool, graphics_queue, device, width, height, zoom);

        triangles_shared_memory_.set_shadow_image(shadow_mapping_->sampler(), shadow_mapping_->image_view());
}

void MeshRenderer::delete_shadow_mapping_buffers()
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        ASSERT(shadow_mapping_);
        shadow_mapping_->delete_buffers();
}

std::vector<vulkan::DescriptorSetLayoutAndBindings> MeshRenderer::mesh_layouts() const
{
        std::vector<vulkan::DescriptorSetLayoutAndBindings> layouts;

        layouts.emplace_back(
                normals_program_.descriptor_set_layout_mesh(), normals_program_.descriptor_set_layout_mesh_bindings());

        layouts.emplace_back(
                points_program_.descriptor_set_layout_mesh(), points_program_.descriptor_set_layout_mesh_bindings());

        layouts.emplace_back(
                triangle_lines_program_.descriptor_set_layout_mesh(),
                triangle_lines_program_.descriptor_set_layout_mesh_bindings());

        layouts.emplace_back(
                triangles_program_.descriptor_set_layout_mesh(),
                triangles_program_.descriptor_set_layout_mesh_bindings());

        if (shadow_mapping_)
        {
                layouts.emplace_back(
                        shadow_mapping_->descriptor_set_layout_mesh(),
                        shadow_mapping_->descriptor_set_layout_mesh_bindings());
        }

        return layouts;
}

std::vector<vulkan::DescriptorSetLayoutAndBindings> MeshRenderer::material_layouts() const
{
        std::vector<vulkan::DescriptorSetLayoutAndBindings> layouts;

        layouts.emplace_back(
                triangles_program_.descriptor_set_layout_material(),
                triangles_program_.descriptor_set_layout_material_bindings());

        return layouts;
}

VkSampler MeshRenderer::texture_sampler() const
{
        return texture_sampler_;
}

void MeshRenderer::draw_commands(
        const std::vector<const MeshObject*>& meshes,
        const VkCommandBuffer command_buffer,
        const bool show_clip_plane_lines,
        const bool show_normals,
        const bool transparent) const
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        if (meshes.empty())
        {
                return;
        }

        const auto& pipelines = render_pipelines(transparent);
        ASSERT(pipelines);

        commands_triangles(
                meshes, command_buffer, pipelines->triangles, transparent, triangles_program_,
                triangles_shared_memory_);

        commands_lines(meshes, command_buffer, pipelines->lines, transparent, points_program_, points_shared_memory_);

        commands_points(meshes, command_buffer, pipelines->points, transparent, points_program_, points_shared_memory_);

        if (show_clip_plane_lines)
        {
                commands_triangle_lines(
                        meshes, command_buffer, pipelines->triangle_lines, transparent, triangle_lines_program_,
                        triangle_lines_shared_memory_);
        }

        if (show_normals)
        {
                commands_normals(
                        meshes, command_buffer, pipelines->normals, transparent, normals_program_,
                        normals_shared_memory_);
        }
}

void MeshRenderer::create_render_command_buffers(
        const std::vector<const MeshObject*>& meshes,
        const VkCommandPool graphics_command_pool,
        const bool show_clip_plane_lines,
        const bool show_normals,
        const std::function<void(VkCommandBuffer command_buffer)>& before_transparency_render_pass_commands,
        const std::function<void(VkCommandBuffer command_buffer)>& after_transparency_render_pass_commands)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        ASSERT(render_buffers_);

        delete_render_command_buffers();

        if (meshes.empty())
        {
                return;
        }

        std::vector<const MeshObject*> opaque_meshes;
        std::vector<const MeshObject*> transparent_meshes;
        find_opaque_and_transparent_meshes(meshes, &opaque_meshes, &transparent_meshes);

        has_opaque_meshes_ = !opaque_meshes.empty();

        vulkan::CommandBufferCreateInfo info;

        info.device = device_;
        info.render_area.emplace();
        info.render_area->offset.x = 0;
        info.render_area->offset.y = 0;
        info.render_area->extent.width = render_buffers_->width();
        info.render_area->extent.height = render_buffers_->height();
        info.render_pass = render_buffers_->render_pass().handle();
        info.framebuffers = &render_buffers_->framebuffers();
        info.command_pool = graphics_command_pool;
        info.clear_values = &render_buffers_->clear_values();

        info.before_render_pass_commands =
                !transparent_meshes.empty() ? before_transparency_render_pass_commands : nullptr;
        info.after_render_pass_commands =
                !transparent_meshes.empty() ? after_transparency_render_pass_commands : nullptr;
        info.render_pass_commands = [&](const VkCommandBuffer command_buffer)
        {
                if (!opaque_meshes.empty())
                {
                        draw_commands(
                                opaque_meshes, command_buffer, show_clip_plane_lines, show_normals,
                                /*transparent=*/false);
                }
                if (!transparent_meshes.empty())
                {
                        draw_commands(
                                transparent_meshes, command_buffer, show_clip_plane_lines, show_normals,
                                /*transparent=*/true);
                }
        };
        command_buffers_all_ = vulkan::create_command_buffers(info);

        if (!transparent_meshes.empty())
        {
                info.before_render_pass_commands = nullptr;
                info.after_render_pass_commands = nullptr;
                info.render_pass_commands = [&](const VkCommandBuffer command_buffer)
                {
                        draw_commands(
                                transparent_meshes, command_buffer, show_clip_plane_lines, show_normals,
                                /*transparent=*/false);
                };
                command_buffers_transparent_as_opaque_ = vulkan::create_command_buffers(info);
        }
}

void MeshRenderer::delete_render_command_buffers()
{
        command_buffers_all_.reset();
        command_buffers_transparent_as_opaque_.reset();
        has_opaque_meshes_ = false;
}

void MeshRenderer::create_shadow_mapping_command_buffers(
        const std::vector<const MeshObject*>& meshes,
        const VkCommandPool graphics_command_pool)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        ASSERT(shadow_mapping_);
        shadow_mapping_->create_command_buffers(device_, meshes, graphics_command_pool);
}

void MeshRenderer::delete_shadow_mapping_command_buffers()
{
        ASSERT(shadow_mapping_);
        shadow_mapping_->delete_command_buffers();
}

void MeshRenderer::set_shadow_matrices(const numerical::Matrix4d& vp_matrix, const numerical::Matrix4d& world_to_shadow)
        const
{
        ASSERT(shadow_mapping_);
        shadow_mapping_->set_shadow_matrices(vp_matrix, world_to_shadow);
}

void MeshRenderer::set_acceleration_structure(const VkAccelerationStructureKHR acceleration_structure)
{
        delete_render_command_buffers();
        triangles_shared_memory_.set_acceleration_structure(acceleration_structure);
}

bool MeshRenderer::has_meshes() const
{
        return command_buffers_all_.has_value();
}

bool MeshRenderer::has_opaque_meshes() const
{
        return has_opaque_meshes_;
}

bool MeshRenderer::has_transparent_meshes() const
{
        return command_buffers_transparent_as_opaque_.has_value();
}

std::optional<VkCommandBuffer> MeshRenderer::render_command_buffer_all(const unsigned index) const
{
        const std::optional<vulkan::handle::CommandBuffers>& command_buffers = command_buffers_all_;
        if (command_buffers)
        {
                ASSERT(index < command_buffers->count());
                return (*command_buffers)[index];
        }
        return std::nullopt;
}

std::optional<VkCommandBuffer> MeshRenderer::render_command_buffer_transparent_as_opaque(const unsigned index) const
{
        const std::optional<vulkan::handle::CommandBuffers>& command_buffers = command_buffers_transparent_as_opaque_;
        if (command_buffers)
        {
                ASSERT(index < command_buffers->count());
                return (*command_buffers)[index];
        }
        return std::nullopt;
}

VkCommandBuffer MeshRenderer::shadow_mapping_command_buffer(const unsigned index) const
{
        ASSERT(shadow_mapping_);
        return shadow_mapping_->command_buffer(index);
}

const vulkan::ImageView& MeshRenderer::shadow_mapping_image_view() const
{
        ASSERT(shadow_mapping_);
        return shadow_mapping_->image_view();
}

VkSampler MeshRenderer::shadow_mapping_sampler() const
{
        ASSERT(shadow_mapping_);
        return shadow_mapping_->sampler();
}
}
