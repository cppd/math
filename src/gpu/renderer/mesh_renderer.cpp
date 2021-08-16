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

#include "mesh_renderer.h"

#include "mesh_sampler.h"

#include <src/com/error.h>
#include <src/vulkan/commands.h>

namespace ns::gpu::renderer
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
                meshes.cbegin(), meshes.cend(),
                [](const MeshObject* mesh)
                {
                        return mesh->transparent();
                });

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
        : device_(device),
          sample_shading_(sample_shading),
          //
          triangles_program_(device),
          triangles_common_memory_(
                  device,
                  triangles_program_.descriptor_set_layout_shared(),
                  triangles_program_.descriptor_set_layout_shared_bindings(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          triangle_lines_program_(device),
          triangle_lines_common_memory_(
                  device,
                  triangle_lines_program_.descriptor_set_layout_shared(),
                  triangle_lines_program_.descriptor_set_layout_shared_bindings(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          normals_program_(device),
          normals_common_memory_(
                  device,
                  normals_program_.descriptor_set_layout_shared(),
                  normals_program_.descriptor_set_layout_shared_bindings(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          triangles_depth_program_(device),
          triangles_depth_common_memory_(
                  device,
                  triangles_depth_program_.descriptor_set_layout_shared(),
                  triangles_depth_program_.descriptor_set_layout_shared_bindings(),
                  buffers.shadow_matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          points_program_(device),
          points_common_memory_(
                  device,
                  points_program_.descriptor_set_layout_shared(),
                  points_program_.descriptor_set_layout_shared_bindings(),
                  buffers.matrices_buffer(),
                  buffers.drawing_buffer()),
          //
          texture_sampler_(create_mesh_texture_sampler(device_, sampler_anisotropy)),
          shadow_sampler_(create_mesh_shadow_sampler(device_))
{
}

const MeshRenderer::Pipelines& MeshRenderer::render_pipelines(bool transparent) const
{
        if (transparent)
        {
                return render_pipelines_transparent_;
        }
        return render_pipelines_opaque_;
}

MeshRenderer::Pipelines& MeshRenderer::render_pipelines(bool transparent)
{
        if (transparent)
        {
                return render_pipelines_transparent_;
        }
        return render_pipelines_opaque_;
}

void MeshRenderer::create_render_buffers(
        const RenderBuffers3D* render_buffers,
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

        triangles_common_memory_.set_objects_image(objects_image);
        triangles_common_memory_.set_transparency(
                transparency_heads_image, transparency_heads_size_image, transparency_counter, transparency_nodes);

        triangle_lines_common_memory_.set_objects_image(objects_image);
        triangle_lines_common_memory_.set_transparency(
                transparency_heads_image, transparency_heads_size_image, transparency_counter, transparency_nodes);

        points_common_memory_.set_objects_image(objects_image);
        points_common_memory_.set_transparency(
                transparency_heads_image, transparency_heads_size_image, transparency_counter, transparency_nodes);

        normals_common_memory_.set_objects_image(objects_image);
        normals_common_memory_.set_transparency(
                transparency_heads_image, transparency_heads_size_image, transparency_counter, transparency_nodes);

        for (bool transparent : {false, true})
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
        ASSERT(thread_id_ == std::this_thread::get_id());

        delete_depth_buffers();

        depth_buffers_ = depth_buffers;

        triangles_common_memory_.set_shadow_texture(shadow_sampler_, depth_buffers->texture(0));

        render_triangles_depth_pipeline_ = triangles_depth_program_.create_pipeline(
                depth_buffers->render_pass(), depth_buffers->sample_count(),
                Region<2, int>(0, 0, depth_buffers->width(), depth_buffers->height()));
}

void MeshRenderer::delete_depth_buffers()
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        delete_depth_command_buffers();

        render_triangles_depth_pipeline_.reset();
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

template <template <typename...> typename T>
void MeshRenderer::draw_commands(
        const T<const MeshObject*>& meshes,
        VkCommandBuffer command_buffer,
        bool clip_plane,
        bool normals,
        bool depth,
        bool transparent) const
{
        ASSERT(thread_id_ == std::this_thread::get_id());

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
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangles_program_.pipeline_layout(),
                        CommonMemory::set_number(), 1 /*set count*/, &triangles_common_memory_.descriptor_set(), 0,
                        nullptr);

                auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set)
                {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangles_program_.pipeline_layout(),
                                MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
                };

                auto bind_descriptor_set_material = [&](VkDescriptorSet descriptor_set)
                {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangles_program_.pipeline_layout(),
                                TrianglesMaterialMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
                };

                for (const MeshObject* mesh : meshes)
                {
                        mesh->commands_triangles(
                                command_buffer, triangles_program_.descriptor_set_layout_mesh(),
                                bind_descriptor_set_mesh, triangles_program_.descriptor_set_layout_material(),
                                bind_descriptor_set_material);
                }
        }
        else
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *render_triangles_depth_pipeline_);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangles_depth_program_.pipeline_layout(),
                        CommonMemory::set_number(), 1 /*set count*/, &triangles_depth_common_memory_.descriptor_set(),
                        0, nullptr);

                auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set)
                {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                triangles_depth_program_.pipeline_layout(), MeshMemory::set_number(), 1 /*set count*/,
                                &descriptor_set, 0, nullptr);
                };

                for (const MeshObject* mesh : meshes)
                {
                        mesh->commands_plain_triangles(
                                command_buffer, triangles_depth_program_.descriptor_set_layout_mesh(),
                                bind_descriptor_set_mesh);
                }
        }

        if (!depth)
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *render_pipelines(transparent).lines);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, points_program_.pipeline_layout(),
                        CommonMemory::set_number(), 1 /*set count*/, &points_common_memory_.descriptor_set(), 0,
                        nullptr);

                auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set)
                {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, points_program_.pipeline_layout(),
                                MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
                };

                for (const MeshObject* mesh : meshes)
                {
                        mesh->commands_lines(
                                command_buffer, points_program_.descriptor_set_layout_mesh(), bind_descriptor_set_mesh);
                }
        }

        if (!depth)
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *render_pipelines(transparent).points);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, points_program_.pipeline_layout(),
                        CommonMemory::set_number(), 1 /*set count*/, &points_common_memory_.descriptor_set(), 0,
                        nullptr);

                auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set)
                {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, points_program_.pipeline_layout(),
                                MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
                };

                for (const MeshObject* mesh : meshes)
                {
                        mesh->commands_points(
                                command_buffer, points_program_.descriptor_set_layout_mesh(), bind_descriptor_set_mesh);
                }
        }

        if (!depth && clip_plane)
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *render_pipelines(transparent).triangle_lines);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, triangle_lines_program_.pipeline_layout(),
                        CommonMemory::set_number(), 1 /*set count*/, &triangle_lines_common_memory_.descriptor_set(), 0,
                        nullptr);

                auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set)
                {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                triangle_lines_program_.pipeline_layout(), MeshMemory::set_number(), 1 /*set count*/,
                                &descriptor_set, 0, nullptr);
                };

                for (const MeshObject* mesh : meshes)
                {
                        mesh->commands_plain_triangles(
                                command_buffer, triangle_lines_program_.descriptor_set_layout_mesh(),
                                bind_descriptor_set_mesh);
                }
        }

        if (!depth && normals)
        {
                vkCmdBindPipeline(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *render_pipelines(transparent).normals);

                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, normals_program_.pipeline_layout(),
                        CommonMemory::set_number(), 1 /*set count*/, &normals_common_memory_.descriptor_set(), 0,
                        nullptr);

                auto bind_descriptor_set_mesh = [&](VkDescriptorSet descriptor_set)
                {
                        vkCmdBindDescriptorSets(
                                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, normals_program_.pipeline_layout(),
                                MeshMemory::set_number(), 1 /*set count*/, &descriptor_set, 0, nullptr);
                };

                for (const MeshObject* mesh : meshes)
                {
                        mesh->commands_triangle_vertices(
                                command_buffer, normals_program_.descriptor_set_layout_mesh(),
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
        ASSERT(thread_id_ == std::this_thread::get_id());

        //

        ASSERT(render_buffers_);

        delete_render_command_buffers();

        if (meshes.empty())
        {
                return;
        }

        std::vector<const MeshObject*> opaque_meshes;
        std::vector<const MeshObject*> transparent_meshes;
        find_opaque_and_transparent(meshes, &opaque_meshes, &transparent_meshes);

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
        info.render_pass_commands = [&](VkCommandBuffer command_buffer)
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
                info.render_pass_commands = [&](VkCommandBuffer command_buffer)
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
        const std::unordered_set<const MeshObject*>& meshes,
        VkCommandPool graphics_command_pool,
        bool clip_plane,
        bool normals)
{
        ASSERT(thread_id_ == std::this_thread::get_id());

        //

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
        info.render_pass_commands = [&](VkCommandBuffer command_buffer)
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

std::optional<VkCommandBuffer> MeshRenderer::render_command_buffer_all(unsigned index) const
{
        if (render_command_buffers_all_)
        {
                ASSERT(index < render_command_buffers_all_->count());
                return (*render_command_buffers_all_)[index];
        }
        return std::nullopt;
}

std::optional<VkCommandBuffer> MeshRenderer::render_command_buffer_transparent_as_opaque(unsigned index) const
{
        if (render_command_buffers_transparent_as_opaque_)
        {
                ASSERT(index < render_command_buffers_transparent_as_opaque_->count());
                return (*render_command_buffers_transparent_as_opaque_)[index];
        }
        return std::nullopt;
}

std::optional<VkCommandBuffer> MeshRenderer::depth_command_buffer(unsigned index) const
{
        if (render_depth_command_buffers_)
        {
                ASSERT(index < render_depth_command_buffers_->count());
                return (*render_depth_command_buffers_)[index];
        }
        return std::nullopt;
}
}
