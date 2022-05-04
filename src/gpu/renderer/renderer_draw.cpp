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

#include "renderer_draw.h"

#include <src/com/error.h>
#include <src/vulkan/error.h>
#include <src/vulkan/queue.h>

namespace ns::gpu::renderer
{
RendererDraw::RendererDraw(
        const VkDevice device,
        const std::uint32_t transparency_node_buffer_max_size,
        const MeshRenderer* const mesh_renderer,
        const VolumeRenderer* const volume_renderer)
        : transparency_node_buffer_max_size_(transparency_node_buffer_max_size),
          transparency_message_(transparency_node_buffer_max_size),
          mesh_semaphore_(device),
          volume_semaphore_(device),
          shadow_mapping_semaphore_(device),
          transparent_as_opaque_semaphore_(device),
          mesh_renderer_(mesh_renderer),
          volume_renderer_(volume_renderer)
{
}

RendererDraw::DrawInfo RendererDraw::draw_meshes(
        VkSemaphore semaphore,
        const vulkan::Queue& graphics_queue,
        const unsigned index,
        const bool shadow_mapping,
        const TransparencyBuffers& transparency_buffers) const
{
        if (!shadow_mapping)
        {
                ASSERT(mesh_renderer_->render_command_buffer_all(index));
                vulkan::queue_submit(
                        semaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        *mesh_renderer_->render_command_buffer_all(index), mesh_semaphore_, graphics_queue);

                semaphore = mesh_semaphore_;
        }
        else
        {
                vulkan::queue_submit(
                        mesh_renderer_->shadow_mapping_command_buffer(index), shadow_mapping_semaphore_,
                        graphics_queue);

                ASSERT(mesh_renderer_->render_command_buffer_all(index));
                vulkan::queue_submit(
                        std::array<VkSemaphore, 2>{semaphore, shadow_mapping_semaphore_},
                        std::array<VkPipelineStageFlags, 2>{
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
                        *mesh_renderer_->render_command_buffer_all(index), mesh_semaphore_, graphics_queue);

                semaphore = mesh_semaphore_;
        }

        if (!mesh_renderer_->has_transparent_meshes())
        {
                transparency_message_.process({});
                return {.semaphore = semaphore, .transparency = false, .opacity = true};
        }

        VULKAN_CHECK(vkQueueWaitIdle(graphics_queue));

        const TransparencyBuffers::Info info = transparency_buffers.read();
        const bool nodes = info.required_node_memory > transparency_node_buffer_max_size_;
        const bool overload = info.overload_counter > 0;

        if (!nodes && !overload)
        {
                transparency_message_.process({});

                return {.semaphore = semaphore, .transparency = true, .opacity = mesh_renderer_->has_opaque_meshes()};
        }

        const auto command_buffer = mesh_renderer_->render_command_buffer_transparent_as_opaque(index);
        ASSERT(command_buffer);
        vulkan::queue_submit(
                semaphore, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, *command_buffer, transparent_as_opaque_semaphore_,
                graphics_queue);

        transparency_message_.process(
                [&]()
                {
                        TransparencyMessage::Data data;
                        if (nodes)
                        {
                                data.required_node_memory = info.required_node_memory;
                        }
                        if (overload)
                        {
                                data.overload_count = info.overload_counter;
                        }
                        return data;
                }());

        return {.semaphore = transparent_as_opaque_semaphore_, .transparency = false, .opacity = true};
}

VkSemaphore RendererDraw::draw(
        const VkSemaphore semaphore,
        const vulkan::Queue& graphics_queue_1,
        const vulkan::Queue& graphics_queue_2,
        const unsigned index,
        const bool shadow_mapping,
        const TransparencyBuffers& transparency_buffers) const
{
        DrawInfo draw_info;

        if (mesh_renderer_->has_meshes())
        {
                draw_info = draw_meshes(semaphore, graphics_queue_1, index, shadow_mapping, transparency_buffers);
        }
        else if (shadow_mapping && volume_renderer_->has_volume())
        {
                vulkan::queue_submit(
                        semaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        mesh_renderer_->shadow_mapping_command_buffer(index), shadow_mapping_semaphore_,
                        graphics_queue_2);

                draw_info = {.semaphore = shadow_mapping_semaphore_, .transparency = false, .opacity = false};
        }
        else
        {
                draw_info = {.semaphore = semaphore, .transparency = false, .opacity = false};
        }

        if (volume_renderer_->has_volume() || draw_info.transparency || draw_info.opacity)
        {
                const auto command_buffer =
                        volume_renderer_->command_buffer(index, draw_info.transparency, draw_info.opacity);
                ASSERT(command_buffer);

                vulkan::queue_submit(
                        draw_info.semaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, *command_buffer, volume_semaphore_,
                        graphics_queue_1);

                draw_info.semaphore = volume_semaphore_;
        }

        return draw_info.semaphore;
}
}
