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

#include "volume_renderer.h"

#include "volume_sampler.h"

#include <src/com/error.h>
#include <src/vulkan/commands.h>

namespace gpu::renderer
{
VolumeRenderer::VolumeRenderer(const vulkan::Device& device, bool sample_shading, const ShaderBuffers& buffers)
        : m_device(device),
          m_sample_shading(sample_shading),
          //
          m_program(device),
          m_memory(
                  device,
                  m_program.descriptor_set_layout_shared(),
                  m_program.descriptor_set_layout_shared_bindings(),
                  buffers.drawing_buffer()),
          //
          m_volume_sampler(create_volume_sampler(m_device)),
          m_depth_sampler(create_volume_depth_image_sampler(m_device))
{
}

void VolumeRenderer::create_buffers(
        const RenderBuffers3D* render_buffers,
        VkCommandPool graphics_command_pool,
        const Region<2, int>& viewport,
        VkImageView depth_image,
        const vulkan::ImageWithMemory& transparency_heads_image,
        const vulkan::Buffer& transparency_nodes)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        delete_buffers();

        m_render_buffers = render_buffers;

        m_memory.set_depth_image(depth_image, m_depth_sampler);
        m_memory.set_transparency(transparency_heads_image, transparency_nodes);

        m_pipeline_volume = m_program.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), m_sample_shading, viewport,
                VolumeProgram::PipelineType::Volume);

        m_pipeline_volume_mesh = m_program.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), m_sample_shading, viewport,
                VolumeProgram::PipelineType::VolumeMesh);

        m_pipeline_mesh = m_program.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), m_sample_shading, viewport,
                VolumeProgram::PipelineType::Mesh);

        create_command_buffers_mesh(graphics_command_pool);
}

void VolumeRenderer::delete_buffers()
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        m_command_buffers_volume.reset();
        m_command_buffers_volume_mesh.reset();
        m_command_buffers_mesh.reset();
        m_pipeline_volume.reset();
        m_pipeline_volume_mesh.reset();
        m_pipeline_mesh.reset();
}

std::vector<vulkan::DescriptorSetLayoutAndBindings> VolumeRenderer::image_layouts() const
{
        std::vector<vulkan::DescriptorSetLayoutAndBindings> layouts;

        layouts.emplace_back(m_program.descriptor_set_layout_image(), m_program.descriptor_set_layout_image_bindings());

        return layouts;
}

VkSampler VolumeRenderer::image_sampler() const
{
        return m_volume_sampler;
}

void VolumeRenderer::draw_commands_mesh(VkCommandBuffer command_buffer) const
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        //

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline_mesh);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_program.pipeline_layout(VolumeProgram::LayoutType::Mesh), VolumeSharedMemory::set_number(),
                1 /*set count*/, &m_memory.descriptor_set(), 0, nullptr);

        vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void VolumeRenderer::create_command_buffers_mesh(VkCommandPool graphics_command_pool)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        //

        ASSERT(m_render_buffers);

        m_command_buffers_mesh.reset();

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
        info.render_pass_commands = [&](VkCommandBuffer command_buffer) { draw_commands_mesh(command_buffer); };

        m_command_buffers_mesh = vulkan::create_command_buffers(info);
}

void VolumeRenderer::draw_commands_volume(const VolumeObject* volume, VkCommandBuffer command_buffer, bool mesh) const
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        //

        if (mesh)
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline_volume_mesh);
        }
        else
        {
                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline_volume);
        }

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_program.pipeline_layout(VolumeProgram::LayoutType::Volume), VolumeSharedMemory::set_number(),
                1 /*set count*/, &m_memory.descriptor_set(), 0, nullptr);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_program.pipeline_layout(VolumeProgram::LayoutType::Volume), VolumeImageMemory::set_number(),
                1 /*set count*/, &volume->descriptor_set(m_program.descriptor_set_layout_image()), 0, nullptr);

        vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void VolumeRenderer::create_command_buffers(
        const VolumeObject* volume,
        VkCommandPool graphics_command_pool,
        const std::function<void(VkCommandBuffer command_buffer)>& before_render_pass_commands)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        //

        ASSERT(m_render_buffers);

        delete_command_buffers();

        if (!volume)
        {
                return;
        }

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
        info.before_render_pass_commands = before_render_pass_commands;

        info.render_pass_commands = [&](VkCommandBuffer command_buffer) {
                draw_commands_volume(volume, command_buffer, false /*mesh*/);
        };
        m_command_buffers_volume = vulkan::create_command_buffers(info);

        info.render_pass_commands = [&](VkCommandBuffer command_buffer) {
                draw_commands_volume(volume, command_buffer, true /*mesh*/);
        };
        m_command_buffers_volume_mesh = vulkan::create_command_buffers(info);
}

void VolumeRenderer::delete_command_buffers()
{
        m_command_buffers_volume.reset();
        m_command_buffers_volume_mesh.reset();
}

bool VolumeRenderer::has_volume() const
{
        ASSERT(static_cast<bool>(m_command_buffers_volume) == static_cast<bool>(m_command_buffers_volume_mesh));

        return static_cast<bool>(m_command_buffers_volume);
}

std::optional<VkCommandBuffer> VolumeRenderer::command_buffer(unsigned index, bool transparency) const
{
        if (has_volume())
        {
                if (transparency)
                {
                        index = m_command_buffers_volume_mesh->count() == 1 ? 0 : index;
                        return (*m_command_buffers_volume_mesh)[index];
                }
                index = m_command_buffers_volume->count() == 1 ? 0 : index;
                return (*m_command_buffers_volume)[index];
        }
        else if (transparency)
        {
                ASSERT(m_command_buffers_mesh);

                index = m_command_buffers_mesh->count() == 1 ? 0 : index;
                return (*m_command_buffers_mesh)[index];
        }
        return std::nullopt;
}
}
