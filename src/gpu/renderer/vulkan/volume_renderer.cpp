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
          m_memory(device, m_program.descriptor_set_layout(), buffers.drawing_buffer()),
          //
          m_volume_sampler(create_volume_sampler(m_device))
{
}

void VolumeRenderer::create_buffers(const RenderBuffers3D* render_buffers, const Region<2, int>& viewport)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        delete_buffers();

        m_render_buffers = render_buffers;

        m_pipeline = m_program.create_pipeline(
                render_buffers->render_pass(), render_buffers->sample_count(), m_sample_shading, viewport);
}

void VolumeRenderer::delete_buffers()
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        m_command_buffers.reset();
        m_pipeline.reset();
}

VolumeDescriptorSetsFunction VolumeRenderer::descriptor_sets_function() const
{
        return [this](const VolumeInfo& info) {
                std::vector<vulkan::Descriptors> sets;

                sets.push_back(VolumeImageMemory::create(
                        m_device, m_volume_sampler, m_volume_sampler, m_program.descriptor_set_layout_image(), info));

                return sets;
        };
}

void VolumeRenderer::draw_commands(const VolumeObject* volume, VkCommandBuffer command_buffer) const
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        //

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_program.pipeline_layout(),
                VolumeMemory::set_number(), 1 /*set count*/, &m_memory.descriptor_set(), 0, nullptr);

        vkCmdBindDescriptorSets(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_program.pipeline_layout(),
                VolumeImageMemory::set_number(), 1 /*set count*/,
                &volume->descriptor_set(m_program.descriptor_set_layout_image()), 0, nullptr);

        vkCmdDraw(command_buffer, 3, 1, 0, 0);
}

void VolumeRenderer::create_command_buffers(
        const VolumeObject* volume,
        VkCommandPool graphics_command_pool,
        const Color& clear_color,
        const std::function<void(VkCommandBuffer command_buffer)>& before_render_pass_commands)
{
        ASSERT(m_thread_id == std::this_thread::get_id());

        //

        ASSERT(m_render_buffers);
        ASSERT(volume);

        m_command_buffers.reset();

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
        info.render_pass_commands = [&](VkCommandBuffer command_buffer) { draw_commands(volume, command_buffer); };

        m_command_buffers = vulkan::create_command_buffers(info);
}

void VolumeRenderer::delete_command_buffers()
{
        m_command_buffers.reset();
}

std::optional<VkCommandBuffer> VolumeRenderer::command_buffer(unsigned index) const
{
        if (m_command_buffers)
        {
                index = m_command_buffers->count() == 1 ? 0 : index;
                return (*m_command_buffers)[index];
        }
        return std::nullopt;
}
}
