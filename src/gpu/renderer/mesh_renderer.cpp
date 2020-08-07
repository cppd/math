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
namespace
{
template <template <typename...> typename T>
void find_opaque_and_transparent(
        const T<const MeshObject*>& meshes,
        std::vector<const MeshObject*>* opaque_meshes,
        std::vector<const MeshObject*>* transparent_meshes)
{
        const unsigned transparent_count = std::count_if(
                meshes.cbegin(), meshes.cend(), [](const MeshObject* mesh) { return mesh->transparent(); });

        opaque_meshes->clear();
        opaque_meshes->reserve(meshes.size() - transparent_count);

        transparent_meshes->clear();
        transparent_meshes->reserve(transparent_count);

        for (const MeshObject* mesh : meshes)
        {
                if (mesh->transparent())
                {
                        transparent_meshes->push_back(mesh);
                }
                else
                {
                        opaque_meshes->push_back(mesh);
                }
        }
}
}

MeshRenderer::MeshRenderer(
        const vulkan::Device& device,
        bool sample_shading,
        bool sampler_anisotropy,
        const ShaderBuffers& buffers)
        : m_device(device),
          m_sample_shading(sample_shading),
          //
          m_triangles_program(device),
          m_triangles_common_memory(
                  device,
                  m_triangles_program.descriptor_set_layout_shared(),
                  m_triangles_program.descriptor_set_layout_shared_bindings(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          m_triangle_lines_program(device),
          m_triangle_lines_common_memory(
                  device,
                  m_triangle_lines_program.descriptor_set_layout_shared(),
                  m_triangle_lines_program.descriptor_set_layout_shared_bindings(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          m_normals_program(device),
          m_normals_common_memory(
                  device,
                  m_normals_program.descriptor_set_layout_shared(),
                  m_normals_program.descriptor_set_layout_shared_bindings(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          m_triangles_depth_program(device),
          m_triangles_depth_common_memory(
                  device,
                  m_triangles_depth_program.descriptor_set_layout_shared(),
                  m_triangles_depth_program.descriptor_set_layout_shared_bindings(),
                  buffers.shadow_matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          m_points_program(device),
          m_points_common_memory(
                  device,
                  m_points_program.descriptor_set_layout_shared(),
                  m_points_program.descriptor_set_layout_shared_bindings(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          m_texture_sampler(create_mesh_texture_sampler(m_device, sampler_anisotropy)),
          m_shadow_sampler(create_mesh_shadow_sampler(m_device))
{
}

const MeshRenderer::Pipelines& MeshRenderer::render_pipelines(bool transparent) const
{
        if (transparent)
        {
                return m_render_pipelines_transparent;
        }
        return m_render_pipelines_opaque;
}

MeshRenderer::Pipelines& MeshRenderer::render_pipelines(bool transparent)
{
        if (transparent)
        {
                return m_render_pipelines_transparent;
        }
        return m_render_pipelines_opaque;
}

void MeshRenderer::create_render_buffers(
        const RenderBuffers3D* render_buffers,
        const vulkan::ImageWithMemory& objects_image,
        const vulkan::ImageWithMemory& transparency_heads_image,
        const vulkan::Buffer& transparency_counter,
        const vulkan::Buffer& transparency_nodes,
        const Region<2, int>& viewport)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        delete_render_buffers();

        m_render_buffers = render_buffers;

        m_triangles_common_memory.set_objects_image(objects_image);
        m_triangles_common_memory.set_transparency(transparency_heads_image, transparency_counter, transparency_nodes);

        m_triangle_lines_common_memory.set_objects_image(objects_image);
        m_triangle_lines_common_memory.set_transparency(
                transparency_heads_image, transparency_counter, transparency_nodes);

        m_points_common_memory.set_objects_image(objects_image);
        m_points_common_memory.set_transparency(transparency_heads_image, transparency_counter, transparency_nodes);

        m_normals_common_memory.set_objects_image(objects_image);
        m_normals_common_memory.set_transparency(transparency_heads_image, transparency_counter, transparency_nodes);

        for (bool transparent : {false, true})
        {
                render_pipelines(transparent).triangles = m_triangles_program.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), m_sample_shading, viewport,
                        transparent);
                render_pipelines(transparent).triangle_lines = m_triangle_lines_program.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), m_sample_shading, viewport,
                        transparent);
                render_pipelines(transparent).normals = m_normals_program.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), m_sample_shading, viewport,
                        transparent);
                render_pipelines(transparent).points = m_points_program.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                        viewport, transparent);
                render_pipelines(transparent).lines = m_points_program.create_pipeline(
                        render_buffers->render_pass(), render_buffers->sample_count(), VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                        viewport, transparent);
        }
}

void MeshRenderer::delete_render_buffers()
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        delete_render_command_buffers();

        for (bool transparent : {false, true})
        {
                render_pipelines(transparent).triangles.reset();
                render_pipelines(transparent).triangle_lines.reset();
                render_pipelines(transparent).normals.reset();
                render_pipelines(transparent).points.reset();
                render_pipelines(transparent).lines.reset();
        }
}

void MeshRenderer::create_depth_buffers(const DepthBuffers* depth_buffers)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        delete_depth_buffers();

        m_depth_buffers = depth_buffers;

        m_triangles_common_memory.set_shadow_texture(m_shadow_sampler, depth_buffers->texture(0));

        m_render_triangles_depth_pipeline = m_triangles_depth_program.create_pipeline(
                depth_buffers->render_pass(), depth_buffers->sample_count(),
                Region<2, int>(0, 0, depth_buffers->width(), depth_buffers->height()));
}

void MeshRenderer::delete_depth_buffers()
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        delete_depth_command_buffers();

        m_render_triangles_depth_pipeline.reset();
}

std::vector<vulkan::DescriptorSetLayoutAndBindings> MeshRenderer::mesh_layouts() const
{
        std::vector<vulkan::DescriptorSetLayoutAndBindings> layouts;

        layouts.emplace_back(
                m_normals_program.descriptor_set_layout_mesh(),
                m_normals_program.descriptor_set_layout_mesh_bindings());

        layouts.emplace_back(
                m_points_program.descriptor_set_layout_mesh(), m_points_program.descriptor_set_layout_mesh_bindings());

        layouts.emplace_back(
                m_triangle_lines_program.descriptor_set_layout_mesh(),
                m_triangle_lines_program.descriptor_set_layout_mesh_bindings());

        layouts.emplace_back(
                m_triangles_program.descriptor_set_layout_mesh(),
                m_triangles_program.descriptor_set_layout_mesh_bindings());

        layouts.emplace_back(
                m_triangles_depth_program.descriptor_set_layout_mesh(),
                m_triangles_depth_program.descriptor_set_layout_mesh_bindings());

        return layouts;
}

std::vector<vulkan::DescriptorSetLayoutAndBindings> MeshRenderer::material_layouts() const
{
        std::vector<vulkan::DescriptorSetLayoutAndBindings> layouts;

        layouts.emplace_back(
                m_triangles_program.descriptor_set_layout_material(),
                m_triangles_program.descriptor_set_layout_material_bindings());

        return layouts;
}

VkSampler MeshRenderer::texture_sampler() const
{
        return m_texture_sampler;
}

template <template <typename...> typename T>
void MeshRenderer::draw_commands(
        const T<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        bool clip_plane,
        bool normals,
        bool depth,
        bool transparent) const
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        //

        if (meshes.empty())
        {
                return;
        }

        ASSERT(!depth || !transparent);

        if (depth)
        {
                vkCmdSetDepthBias(command_buffer, 1.5f, 0.0f, 1.5f);
        }

        if (!depth)
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *render_pipelines(transparent).triangles);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_triangles_program.pipeline_layout(),
                        CommonMemory::set_number(), 1 /*set count*/, &m_triangles_common_memory.descriptor_set(), 0,
                        nullptr);

                auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set) {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_triangles_program.pipeline_layout(),
                                MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
                };

                auto bind_descriptor_set_material = [&](VkDescriptorSet descriptor_set) {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_triangles_program.pipeline_layout(),
                                TrianglesMaterialMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
                };

                for (const MeshObject* mesh : meshes)
                {
                        mesh->commands_triangles(
                                command_buffer, m_triangles_program.descriptor_set_layout_mesh(),
                                bind_descriptor_set_mesh, m_triangles_program.descriptor_set_layout_material(),
                                bind_descriptor_set_material);
                }
        }
        else
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_render_triangles_depth_pipeline);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_triangles_depth_program.pipeline_layout(),
                        CommonMemory::set_number(), 1 /*set count*/, &m_triangles_depth_common_memory.descriptor_set(),
                        0, nullptr);

                auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set) {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_triangles_depth_program.pipeline_layout(), MeshMemory::set_number(), 1 /*set count*/,
                                &descriptor_set, 0, nullptr);
                };

                for (const MeshObject* mesh : meshes)
                {
                        mesh->commands_plain_triangles(
                                command_buffer, m_triangles_depth_program.descriptor_set_layout_mesh(),
                                bind_descriptor_set_mesh);
                }
        }

        if (!depth)
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *render_pipelines(transparent).lines);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_points_program.pipeline_layout(),
                        CommonMemory::set_number(), 1 /*set count*/, &m_points_common_memory.descriptor_set(), 0,
                        nullptr);

                auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set) {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_points_program.pipeline_layout(),
                                MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
                };

                for (const MeshObject* mesh : meshes)
                {
                        mesh->commands_lines(
                                command_buffer, m_points_program.descriptor_set_layout_mesh(),
                                bind_descriptor_set_mesh);
                }
        }

        if (!depth)
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *render_pipelines(transparent).points);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_points_program.pipeline_layout(),
                        CommonMemory::set_number(), 1 /*set count*/, &m_points_common_memory.descriptor_set(), 0,
                        nullptr);

                auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set) {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_points_program.pipeline_layout(),
                                MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
                };

                for (const MeshObject* mesh : meshes)
                {
                        mesh->commands_points(
                                command_buffer, m_points_program.descriptor_set_layout_mesh(),
                                bind_descriptor_set_mesh);
                }
        }

        if (!depth && clip_plane)
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *render_pipelines(transparent).triangle_lines);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_triangle_lines_program.pipeline_layout(),
                        CommonMemory::set_number(), 1 /*set count*/, &m_triangle_lines_common_memory.descriptor_set(),
                        0, nullptr);

                auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set) {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_triangle_lines_program.pipeline_layout(), MeshMemory::set_number(), 1 /*set count*/,
                                &descriptor_set, 0, nullptr);
                };

                for (const MeshObject* mesh : meshes)
                {
                        mesh->commands_plain_triangles(
                                command_buffer, m_triangle_lines_program.descriptor_set_layout_mesh(),
                                bind_descriptor_set_mesh);
                }
        }

        if (!depth && normals)
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *render_pipelines(transparent).normals);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_normals_program.pipeline_layout(),
                        CommonMemory::set_number(), 1 /*set count*/, &m_normals_common_memory.descriptor_set(), 0,
                        nullptr);

                auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set) {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_normals_program.pipeline_layout(),
                                MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
                };

                for (const MeshObject* mesh : meshes)
                {
                        mesh->commands_triangle_vertices(
                                command_buffer, m_normals_program.descriptor_set_layout_mesh(),
                                bind_descriptor_set_mesh);
                }
        }
}

void MeshRenderer::create_render_command_buffers(
        const std::unordered_set<const MeshObject*>& meshes,
        VkCommandPool graphics_command_pool,
        bool clip_plane,
        bool normals,
        const std::function<void(VkCommandBuffer command_buffer)>& before_transparency_render_pass_commands,
        const std::function<void(VkCommandBuffer command_buffer)>& after_transparency_render_pass_commands)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        //

        ASSERT(m_render_buffers);

        delete_render_command_buffers();

        if (meshes.empty())
        {
                return;
        }

        std::vector<const MeshObject*> opaque_meshes;
        std::vector<const MeshObject*> transparent_meshes;
        find_opaque_and_transparent(meshes, &opaque_meshes, &transparent_meshes);

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

        info.before_render_pass_commands =
                !transparent_meshes.empty() ? before_transparency_render_pass_commands : nullptr;
        info.render_pass_commands = [&](VkCommandBuffer command_buffer) {
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
        m_render_command_buffers_all = vulkan::create_command_buffers(info);

        if (!transparent_meshes.empty())
        {
                info.before_render_pass_commands = nullptr;
                info.render_pass_commands = [&](VkCommandBuffer command_buffer) {
                        draw_commands(
                                transparent_meshes, command_buffer, clip_plane, normals, false /*depth*/,
                                false /*transparent_pipeline*/);
                };
                info.after_render_pass_commands = nullptr;
                m_render_command_buffers_transparent_as_opaque = vulkan::create_command_buffers(info);
        }
}

void MeshRenderer::delete_render_command_buffers()
{
        m_render_command_buffers_all.reset();
        m_render_command_buffers_transparent_as_opaque.reset();
}

void MeshRenderer::create_depth_command_buffers(
        const std::unordered_set<const MeshObject*>& meshes,
        VkCommandPool graphics_command_pool,
        bool clip_plane,
        bool normals)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        //

        ASSERT(m_depth_buffers);

        delete_depth_command_buffers();

        if (meshes.empty())
        {
                return;
        }

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
                draw_commands(
                        meshes, command_buffer, clip_plane, normals, true /*depth*/, false /*transparent_pipeline*/);
        };

        m_render_depth_command_buffers = vulkan::create_command_buffers(info);
}

void MeshRenderer::delete_depth_command_buffers()
{
        m_render_depth_command_buffers.reset();
}

bool MeshRenderer::has_meshes() const
{
        return m_render_command_buffers_all.has_value();
}

bool MeshRenderer::has_transparent_meshes() const
{
        return m_render_command_buffers_transparent_as_opaque.has_value();
}

std::optional<VkCommandBuffer> MeshRenderer::render_command_buffer_all(unsigned index) const
{
        if (m_render_command_buffers_all)
        {
                index = m_render_command_buffers_all->count() == 1 ? 0 : index;
                return (*m_render_command_buffers_all)[index];
        }
        return std::nullopt;
}

std::optional<VkCommandBuffer> MeshRenderer::render_command_buffer_transparent_as_opaque(unsigned index) const
{
        if (m_render_command_buffers_transparent_as_opaque)
        {
                index = m_render_command_buffers_transparent_as_opaque->count() == 1 ? 0 : index;
                return (*m_render_command_buffers_transparent_as_opaque)[index];
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
