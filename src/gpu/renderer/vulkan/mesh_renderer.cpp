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

#include "mesh_renderer.h"

#include "mesh_sampler.h"

#include <src/com/error.h>
#include <src/vulkan/commands.h>

namespace gpu::renderer
{
MeshRenderer::MeshRenderer(
        const vulkan::Device& device,
        bool sample_shading,
        bool sampler_anisotropy,
        const ShaderBuffers& buffers)
        : m_device(device),
          m_sample_shading(sample_shading),
          //
          m_triangles_program(device),
          m_triangles_memory(
                  device,
                  m_triangles_program.descriptor_set_layout(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          m_triangle_lines_program(device),
          m_triangle_lines_memory(
                  device,
                  m_triangle_lines_program.descriptor_set_layout(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          m_normals_program(device),
          m_normals_memory(
                  device,
                  m_normals_program.descriptor_set_layout(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          m_triangles_depth_program(device),
          m_triangles_depth_memory(
                  device,
                  m_triangles_depth_program.descriptor_set_layout(),
                  buffers.shadow_matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          m_points_program(device),
          m_points_memory(
                  device,
                  m_points_program.descriptor_set_layout(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          m_texture_sampler(create_mesh_texture_sampler(m_device, sampler_anisotropy)),
          m_shadow_sampler(create_mesh_shadow_sampler(m_device))
{
}

void MeshRenderer::create_render_buffers(
        const RenderBuffers3D* render_buffers,
        const vulkan::ImageWithMemory* object_image,
        const Region<2, int>& viewport)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        delete_render_buffers();

        m_render_buffers = render_buffers;

        m_triangles_memory.set_object_image(object_image);
        m_points_memory.set_object_image(object_image);

        m_render_triangles_pipeline = m_triangles_program.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), m_sample_shading, viewport);
        m_render_triangle_lines_pipeline = m_triangle_lines_program.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), m_sample_shading, viewport);
        m_render_normals_pipeline = m_normals_program.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), m_sample_shading, viewport);
        m_render_points_pipeline = m_points_program.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                viewport);
        m_render_lines_pipeline = m_points_program.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                viewport);
}

void MeshRenderer::delete_render_buffers()
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        m_render_command_buffers.reset();

        m_render_triangles_pipeline.reset();
        m_render_triangle_lines_pipeline.reset();
        m_render_normals_pipeline.reset();
        m_render_points_pipeline.reset();
        m_render_lines_pipeline.reset();
}

void MeshRenderer::create_depth_buffers(const DepthBuffers* depth_buffers)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        delete_depth_buffers();

        m_depth_buffers = depth_buffers;

        m_triangles_memory.set_shadow_texture(m_shadow_sampler, depth_buffers->texture(0));

        m_render_triangles_depth_pipeline = m_triangles_depth_program.create_pipeline(
                depth_buffers->render_pass(), depth_buffers->sample_count(),
                Region<2, int>(0, 0, depth_buffers->width(), depth_buffers->height()));
}

void MeshRenderer::delete_depth_buffers()
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        m_render_depth_command_buffers.reset();

        m_render_triangles_depth_pipeline.reset();
}

void MeshRenderer::create_descriptor_sets(MeshObject* mesh) const
{
        mesh->create_descriptor_sets([&](const std::vector<MaterialInfo>& materials) {
                return TrianglesMaterialMemory::create(
                        m_device, m_texture_sampler, m_triangles_program.descriptor_set_layout_material(), materials);
        });
}

void MeshRenderer::draw_commands(
        const MeshObject* mesh,
        VkCommandBuffer command_buffer,
        bool clip_plane,
        bool normals,
        bool depth) const
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        //

        if (depth)
        {
                vkCmdSetDepthBias(command_buffer, 1.5f, 0.0f, 1.5f);
        }

        if (!depth)
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_triangles_pipeline);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_triangles_program.pipeline_layout(),
                        TrianglesMemory::set_number(), 1 /*set count*/, &m_triangles_memory.descriptor_set(), 0,
                        nullptr);

                auto bind_material_descriptor_set = [&](VkDescriptorSet descriptor_set) {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_triangles_program.pipeline_layout(),
                                TrianglesMaterialMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
                };

                mesh->commands_triangles(
                        command_buffer, m_triangles_program.descriptor_set_layout_material(),
                        bind_material_descriptor_set);
        }
        else
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_triangles_depth_pipeline);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_triangles_depth_program.pipeline_layout(),
                        TrianglesDepthMemory::set_number(), 1 /*set count*/, &m_triangles_depth_memory.descriptor_set(),
                        0, nullptr);

                mesh->commands_plain_triangles(command_buffer);
        }

        if (!depth)
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_lines_pipeline);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_points_program.pipeline_layout(),
                        PointsMemory::set_number(), 1 /*set count*/, &m_points_memory.descriptor_set(), 0, nullptr);

                mesh->commands_lines(command_buffer);
        }

        if (!depth)
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_points_pipeline);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_points_program.pipeline_layout(),
                        PointsMemory::set_number(), 1 /*set count*/, &m_points_memory.descriptor_set(), 0, nullptr);

                mesh->commands_points(command_buffer);
        }

        if (!depth && clip_plane)
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_triangle_lines_pipeline);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_triangle_lines_program.pipeline_layout(),
                        TriangleLinesMemory::set_number(), 1 /*set count*/, &m_triangle_lines_memory.descriptor_set(),
                        0, nullptr);

                mesh->commands_plain_triangles(command_buffer);
        }

        if (!depth && normals)
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_normals_pipeline);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_normals_program.pipeline_layout(),
                        NormalsMemory::set_number(), 1 /*set count*/, &m_normals_memory.descriptor_set(), 0, nullptr);

                mesh->commands_triangle_vertices(command_buffer);
        }
}

void MeshRenderer::create_render_command_buffers(
        const MeshObject* mesh,
        VkCommandPool graphics_command_pool,
        bool clip_plane,
        bool normals,
        const Color& clear_color,
        const std::function<void(VkCommandBuffer command_buffer)>& before_render_pass_commands)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        //

        ASSERT(m_render_buffers);
        ASSERT(mesh);

        m_render_command_buffers.reset();

        vulkan::CommandBufferCreateInfo info;

        info.device = m_device;
        info.render_area.emplace();
        info.render_area->offset.x = 0;
        info.render_area->offset.y = 0;
        info.render_area->extent.width = m_render_buffers->width();
        info.render_area->extent.height = m_render_buffers->height();
        info.render_pass = m_render_buffers->render_pass();
        info.framebuffers = &m_render_buffers->framebuffers();
        info.command_pool = graphics_command_pool;
        const std::vector<VkClearValue> clear_values = m_render_buffers->clear_values(clear_color);
        info.clear_values = &clear_values;
        info.before_render_pass_commands = before_render_pass_commands;
        info.render_pass_commands = [&](VkCommandBuffer command_buffer) {
                draw_commands(mesh, command_buffer, clip_plane, normals, false /*depth*/);
        };

        m_render_command_buffers = vulkan::create_command_buffers(info);
}

void MeshRenderer::delete_render_command_buffers()
{
        m_render_command_buffers.reset();
}

void MeshRenderer::create_depth_command_buffers(
        const MeshObject* mesh,
        VkCommandPool graphics_command_pool,
        bool clip_plane,
        bool normals)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        //

        ASSERT(m_depth_buffers);
        ASSERT(mesh);

        m_render_depth_command_buffers.reset();

        vulkan::CommandBufferCreateInfo info;

        info.device = m_device;
        info.render_area.emplace();
        info.render_area->offset.x = 0;
        info.render_area->offset.y = 0;
        info.render_area->extent.width = m_depth_buffers->width();
        info.render_area->extent.height = m_depth_buffers->height();
        info.render_pass = m_depth_buffers->render_pass();
        info.framebuffers = &m_depth_buffers->framebuffers();
        info.command_pool = graphics_command_pool;
        info.clear_values = &m_depth_buffers->clear_values();
        info.render_pass_commands = [&](VkCommandBuffer command_buffer) {
                draw_commands(mesh, command_buffer, clip_plane, normals, true /*depth*/);
        };

        m_render_depth_command_buffers = vulkan::create_command_buffers(info);
}

void MeshRenderer::delete_depth_command_buffers()
{
        m_render_depth_command_buffers.reset();
}

std::optional<VkCommandBuffer> MeshRenderer::render_command_buffer(unsigned index) const
{
        if (m_render_command_buffers)
        {
                index = m_render_command_buffers->count() == 1 ? 0 : index;
                return (*m_render_command_buffers)[index];
        }
        return std::nullopt;
}

std::optional<VkCommandBuffer> MeshRenderer::depth_command_buffer(unsigned index) const
{
        if (m_render_depth_command_buffers)
        {
                index = m_render_depth_command_buffers->count() == 1 ? 0 : index;
                return (*m_render_depth_command_buffers)[index];
        }
        return std::nullopt;
}
}
