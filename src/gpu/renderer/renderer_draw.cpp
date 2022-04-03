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

std::tuple<VkSemaphore, bool> RendererDraw::draw_meshes(
        VkSemaphore semaphore,
        const vulkan::Queue& graphics_queue,
        const unsigned index,
        const bool shadow_mapping,
        const TransparencyBuffers& transparency_buffers) const
{
        const bool has_volume = volume_renderer_->has_volume();

        if (!shadow_mapping)
        {
                ASSERT(mesh_renderer_->render_command_buffer_all(index, has_volume));
                vulkan::queue_submit(
                        semaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        *mesh_renderer_->render_command_buffer_all(index, has_volume), mesh_semaphore_, graphics_queue);

                semaphore = mesh_semaphore_;
        }
        else
        {
                vulkan::queue_submit(
                        mesh_renderer_->shadow_mapping_command_buffer(index), shadow_mapping_semaphore_,
                        graphics_queue);

                ASSERT(mesh_renderer_->render_command_buffer_all(index, has_volume));
                vulkan::queue_submit(
                        std::array<VkSemaphore, 2>{semaphore, shadow_mapping_semaphore_},
                        std::array<VkPipelineStageFlags, 2>{
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
                        *mesh_renderer_->render_command_buffer_all(index, has_volume), mesh_semaphore_, graphics_queue);

                semaphore = mesh_semaphore_;
        }

        if (!mesh_renderer_->has_transparent_meshes())
        {
                transparency_message_.process(-1, -1);
                return {semaphore, false /*transparency*/};
        }

        VULKAN_CHECK(vkQueueWaitIdle(graphics_queue));

        unsigned long long required_node_memory;
        unsigned overload_counter;
        transparency_buffers.read(&required_node_memory, &overload_counter);

        const bool nodes = required_node_memory > transparency_node_buffer_max_size_;
        const bool overload = overload_counter > 0;
        bool transparency;
        if (nodes || overload)
        {
                ASSERT(mesh_renderer_->render_command_buffer_transparent_as_opaque(index, has_volume));
                vulkan::queue_submit(
                        semaphore, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                        *mesh_renderer_->render_command_buffer_transparent_as_opaque(index, has_volume),
                        transparent_as_opaque_semaphore_, graphics_queue);

                semaphore = transparent_as_opaque_semaphore_;
                transparency = false;
        }
        else
        {
                transparency = true;
        }

        transparency_message_.process(
                nodes ? static_cast<long long>(required_node_memory) : -1,
                overload ? static_cast<long long>(overload_counter) : -1);

        return {semaphore, transparency};
}

VkSemaphore RendererDraw::draw(
        VkSemaphore semaphore,
        const vulkan::Queue& graphics_queue_1,
        const vulkan::Queue& graphics_queue_2,
        const unsigned index,
        const bool shadow_mapping,
        const TransparencyBuffers& transparency_buffers) const
{
        bool transparency = false;

        if (mesh_renderer_->has_meshes())
        {
                std::tie(semaphore, transparency) =
                        draw_meshes(semaphore, graphics_queue_1, index, shadow_mapping, transparency_buffers);
        }
        else if (shadow_mapping && volume_renderer_->has_volume())
        {
                vulkan::queue_submit(
                        semaphore, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        mesh_renderer_->shadow_mapping_command_buffer(index), shadow_mapping_semaphore_,
                        graphics_queue_2);
                semaphore = shadow_mapping_semaphore_;
                transparency = false;
        }

        if (volume_renderer_->has_volume() || transparency)
        {
                ASSERT(volume_renderer_->command_buffer(index, transparency));
                vulkan::queue_submit(
                        semaphore, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        *volume_renderer_->command_buffer(index, transparency), volume_semaphore_, graphics_queue_1);

                semaphore = volume_semaphore_;
        }

        return semaphore;
}
}
