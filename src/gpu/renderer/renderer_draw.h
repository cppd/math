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

#pragma once

#include "mesh_renderer.h"
#include "transparency_message.h"
#include "volume_renderer.h"

#include "buffers/transparency.h"

#include <src/vulkan/objects.h>

namespace ns::gpu::renderer
{
class RendererDraw
{
        const std::uint32_t transparency_node_buffer_max_size_;

        mutable TransparencyMessage transparency_message_;

        vulkan::handle::Semaphore mesh_semaphore_;
        vulkan::handle::Semaphore volume_semaphore_;
        vulkan::handle::Semaphore mesh_depth_semaphore_;
        vulkan::handle::Semaphore clear_semaphore_;
        vulkan::handle::Semaphore transparent_as_opaque_semaphore_;

        const MeshRenderer* mesh_renderer_;
        const VolumeRenderer* volume_renderer_;

        std::tuple<VkSemaphore, bool> draw_meshes(
                VkSemaphore semaphore,
                const vulkan::Queue& graphics_queue,
                unsigned index,
                bool show_shadow,
                const TransparencyBuffers& transparency_buffers) const;

public:
        RendererDraw(
                VkDevice device,
                std::uint32_t transparency_node_buffer_max_size,
                const MeshRenderer* mesh_renderer,
                const VolumeRenderer* volume_renderer);

        VkSemaphore draw(
                const vulkan::Queue& graphics_queue_1,
                const vulkan::Queue& graphics_queue_2,
                unsigned index,
                bool show_shadow,
                const vulkan::handle::CommandBuffers& clear_command_buffers,
                const TransparencyBuffers& transparency_buffers) const;
};
}
