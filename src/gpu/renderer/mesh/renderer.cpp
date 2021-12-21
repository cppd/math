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

#include "renderer.h"

#include "commands.h"
#include "meshes.h"
#include "sampler.h"

#include <src/com/error.h>
#include <src/vulkan/commands.h>

namespace ns::gpu::renderer
{
MeshRenderer::MeshRenderer(
        const vulkan::Device* const device,
        const bool sample_shading,
        const bool sampler_anisotropy,
        const ShaderBuffers& buffers)
        : device_(*device),
          sample_shading_(sample_shading),
          //
          triangles_program_(device),
          triangles_common_memory_(
                  *device,
                  triangles_program_.descriptor_set_layout_shared(),
                  triangles_program_.descriptor_set_layout_shared_bindings(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          triangle_lines_program_(device),
          triangle_lines_common_memory_(
                  *device,
                  triangle_lines_program_.descriptor_set_layout_shared(),
                  triangle_lines_program_.descriptor_set_layout_shared_bindings(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          normals_program_(device),
          normals_common_memory_(
                  *device,
                  normals_program_.descriptor_set_layout_shared(),
                  normals_program_.descriptor_set_layout_shared_bindings(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          triangles_depth_program_(device),
          triangles_depth_common_memory_(
                  *device,
                  triangles_depth_program_.descriptor_set_layout_shared(),
                  triangles_depth_program_.descriptor_set_layout_shared_bindings(),
                  buffers.shadow_matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          points_program_(device),
          points_common_memory_(
                  *device,
                  points_program_.descriptor_set_layout_shared(),
                  points_program_.descriptor_set_layout_shared_bindings(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          texture_sampler_(create_mesh_texture_sampler(*device, sampler_anisotropy)),
          shadow_sampler_(create_mesh_shadow_sampler(*device))
{
}

const MeshRenderer::Pipelines& MeshRenderer::render_pipelines(const bool transparent) const
{
        if (transparent)
        {
                return render_pipelines_transparent_;
        }
        return render_pipelines_opaque_;
}

MeshRenderer::Pipelines& MeshRenderer::render_pipelines(const bool transparent)
{
        if (transparent)
        {
                return render_pipelines_transparent_;
        }
        return render_pipelines_opaque_;
}

void MeshRenderer::create_render_buffers(
        const RenderBuffers3D* const render_buffers,
        const vulkan::ImageWithMemory& objects_image,
        const vulkan::ImageWithMemory& transparency_heads_image,
        const vulkan::ImageWithMemory& transparency_heads_size_image,
        const vulkan::Buffer& transparency_counter,
        const vulkan::Buffer& transparency_nodes,
        const Region<2, int>& viewport)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        delete_render_buffers();

        render_buffers_ = render_buffers;

        triangles_common_memory_.set_objects_image(objects_image.image_view());
        triangles_common_memory_.set_transparency(
                transparency_heads_image.image_view(), transparency_heads_size_image.image_view(), transparency_counter,
                transparency_nodes);

        triangle_lines_common_memory_.set_objects_image(objects_image.image_view());
        triangle_lines_common_memory_.set_transparency(
                transparency_heads_image.image_view(), transparency_heads_size_image.image_view(), transparency_counter,
                transparency_nodes);

        points_common_memory_.set_objects_image(objects_image.image_view());
        points_common_memory_.set_transparency(
                transparency_heads_image.image_view(), transparency_heads_size_image.image_view(), transparency_counter,
                transparency_nodes);

        normals_common_memory_.set_objects_image(objects_image.image_view());
        normals_common_memory_.set_transparency(
                transparency_heads_image.image_view(), transparency_heads_size_image.image_view(), transparency_counter,
                transparency_nodes);

        for (const bool transparent : {false, true})
        {
                render_pipelines(transparent).triangles = triangles_program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), sample_shading_, viewport,
                        transparent);
                render_pipelines(transparent).triangle_lines = triangle_lines_program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), sample_shading_, viewport,
                        transparent);
                render_pipelines(transparent).normals = normals_program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), sample_shading_, viewport,
                        transparent);
                render_pipelines(transparent).points = points_program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                        viewport, transparent);
                render_pipelines(transparent).lines = points_program_.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                        viewport, transparent);
        }
}

void MeshRenderer::delete_render_buffers()
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        delete_render_command_buffers();

        for (const bool transparent : {false, true})
        {
                render_pipelines(transparent).triangles.reset();
                render_pipelines(transparent).triangle_lines.reset();
                render_pipelines(transparent).normals.reset();
                render_pipelines(transparent).points.reset();
                render_pipelines(transparent).lines.reset();
        }
}

void MeshRenderer::create_depth_buffers(
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

        delete_depth_buffers();

        depth_buffers_ = renderer::create_depth_buffers(
                buffer_count, family_indices, graphics_command_pool, graphics_queue, device, width, height, zoom);

        triangles_common_memory_.set_shadow_image(shadow_sampler_, depth_buffers_->image_view(0));

        render_triangles_depth_pipeline_ = triangles_depth_program_.create_pipeline(
                depth_buffers_->render_pass(), depth_buffers_->sample_count(),
                Region<2, int>({0, 0}, {depth_buffers_->width(), depth_buffers_->height()}));
}

void MeshRenderer::delete_depth_buffers()
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        delete_depth_command_buffers();

        render_triangles_depth_pipeline_.reset();

        depth_buffers_.reset();
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

        layouts.emplace_back(
                triangles_depth_program_.descriptor_set_layout_mesh(),
                triangles_depth_program_.descriptor_set_layout_mesh_bindings());

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
        const bool clip_plane,
        const bool normals,
        const bool depth,
        const bool transparent) const
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        if (meshes.empty())
        {
                return;
        }

        if (depth)
        {
                ASSERT(!transparent);

                vkCmdSetDepthBias(command_buffer, 1.5f, 0.0f, 1.5f);

                commands_depth_triangles(
                        meshes, command_buffer, *render_triangles_depth_pipeline_, triangles_depth_program_,
                        triangles_depth_common_memory_);

                return;
        }

        commands_triangles(
                meshes, command_buffer, *render_pipelines(transparent).triangles, triangles_program_,
                triangles_common_memory_);

        commands_lines(
                meshes, command_buffer, *render_pipelines(transparent).lines, points_program_, points_common_memory_);

        commands_points(
                meshes, command_buffer, *render_pipelines(transparent).points, points_program_, points_common_memory_);

        if (clip_plane)
        {
                commands_triangle_lines(
                        meshes, command_buffer, *render_pipelines(transparent).triangle_lines, triangle_lines_program_,
                        triangle_lines_common_memory_);
        }

        if (normals)
        {
                commands_normals(
                        meshes, command_buffer, *render_pipelines(transparent).normals, normals_program_,
                        normals_common_memory_);
        }
}

void MeshRenderer::create_render_command_buffers(
        const std::vector<const MeshObject*>& meshes,
        const VkCommandPool graphics_command_pool,
        const bool clip_plane,
        const bool normals,
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

        vulkan::CommandBufferCreateInfo info;

        info.device = device_;
        info.render_area.emplace();
        info.render_area->offset.x = 0;
        info.render_area->offset.y = 0;
        info.render_area->extent.width = render_buffers_->width();
        info.render_area->extent.height = render_buffers_->height();
        info.render_pass = render_buffers_->render_pass();
        info.framebuffers = &render_buffers_->framebuffers();
        info.command_pool = graphics_command_pool;

        info.before_render_pass_commands =
                !transparent_meshes.empty() ? before_transparency_render_pass_commands : nullptr;
        info.render_pass_commands = [&](const VkCommandBuffer command_buffer)
        {
                if (!opaque_meshes.empty())
                {
                        draw_commands(
                                opaque_meshes, command_buffer, clip_plane, normals, false /*depth*/,
                                false /*transparent_pipeline*/);
                }
                if (!transparent_meshes.empty())
                {
                        draw_commands(
                                transparent_meshes, command_buffer, clip_plane, normals, false /*depth*/,
                                true /*transparent_pipeline*/);
                }
        };
        info.after_render_pass_commands =
                !transparent_meshes.empty() ? after_transparency_render_pass_commands : nullptr;
        render_command_buffers_all_ = vulkan::create_command_buffers(info);

        if (!transparent_meshes.empty())
        {
                info.before_render_pass_commands = nullptr;
                info.render_pass_commands = [&](const VkCommandBuffer command_buffer)
                {
                        draw_commands(
                                transparent_meshes, command_buffer, clip_plane, normals, false /*depth*/,
                                false /*transparent_pipeline*/);
                };
                info.after_render_pass_commands = nullptr;
                render_command_buffers_transparent_as_opaque_ = vulkan::create_command_buffers(info);
        }
}

void MeshRenderer::delete_render_command_buffers()
{
        render_command_buffers_all_.reset();
        render_command_buffers_transparent_as_opaque_.reset();
}

void MeshRenderer::create_depth_command_buffers(
        const std::vector<const MeshObject*>& meshes,
        const VkCommandPool graphics_command_pool,
        const bool clip_plane,
        const bool normals)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        ASSERT(depth_buffers_);

        delete_depth_command_buffers();

        if (meshes.empty())
        {
                return;
        }

        vulkan::CommandBufferCreateInfo info;

        info.device = device_;
        info.render_area.emplace();
        info.render_area->offset.x = 0;
        info.render_area->offset.y = 0;
        info.render_area->extent.width = depth_buffers_->width();
        info.render_area->extent.height = depth_buffers_->height();
        info.render_pass = depth_buffers_->render_pass();
        info.framebuffers = &depth_buffers_->framebuffers();
        info.command_pool = graphics_command_pool;
        info.clear_values = &depth_buffers_->clear_values();
        info.render_pass_commands = [&](const VkCommandBuffer command_buffer)
        {
                draw_commands(
                        meshes, command_buffer, clip_plane, normals, true /*depth*/, false /*transparent_pipeline*/);
        };

        render_depth_command_buffers_ = vulkan::create_command_buffers(info);
}

void MeshRenderer::delete_depth_command_buffers()
{
        render_depth_command_buffers_.reset();
}

bool MeshRenderer::has_meshes() const
{
        return render_command_buffers_all_.has_value();
}

bool MeshRenderer::has_transparent_meshes() const
{
        return render_command_buffers_transparent_as_opaque_.has_value();
}

std::optional<VkCommandBuffer> MeshRenderer::render_command_buffer_all(const unsigned index) const
{
        if (render_command_buffers_all_)
        {
                ASSERT(index < render_command_buffers_all_->count());
                return (*render_command_buffers_all_)[index];
        }
        return std::nullopt;
}

std::optional<VkCommandBuffer> MeshRenderer::render_command_buffer_transparent_as_opaque(const unsigned index) const
{
        if (render_command_buffers_transparent_as_opaque_)
        {
                ASSERT(index < render_command_buffers_transparent_as_opaque_->count());
                return (*render_command_buffers_transparent_as_opaque_)[index];
        }
        return std::nullopt;
}

std::optional<VkCommandBuffer> MeshRenderer::depth_command_buffer(const unsigned index) const
{
        if (render_depth_command_buffers_)
        {
                ASSERT(index < render_depth_command_buffers_->count());
                return (*render_depth_command_buffers_)[index];
        }
        return std::nullopt;
}
}
