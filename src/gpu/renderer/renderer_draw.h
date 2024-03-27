/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "transparency_message.h"

#include "buffers/transparency.h"
#include "mesh/renderer.h"
#include "volume/renderer.h"

#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>

namespace ns::gpu::renderer
{
class RendererDraw final
{
        const std::uint32_t transparency_node_buffer_max_size_;

        mutable TransparencyMessage transparency_message_;

        vulkan::handle::Semaphore mesh_semaphore_;
        vulkan::handle::Semaphore volume_semaphore_;
        vulkan::handle::Semaphore shadow_mapping_semaphore_;
        vulkan::handle::Semaphore transparent_as_opaque_semaphore_;

        const MeshRenderer* mesh_renderer_;
        const VolumeRenderer* volume_renderer_;

        struct DrawInfo final
        {
                VkSemaphore semaphore;
                bool opacity;
                bool transparency;
        };

        [[nodiscard]] DrawInfo draw_meshes(
                VkSemaphore semaphore,
                VkQueue graphics_queue,
                unsigned index,
                bool shadow_mapping,
                const TransparencyBuffers& transparency_buffers) const;

public:
        RendererDraw(
                VkDevice device,
                std::uint32_t transparency_node_buffer_max_size,
                const MeshRenderer* mesh_renderer,
                const VolumeRenderer* volume_renderer);

        [[nodiscard]] VkSemaphore draw(
                VkSemaphore semaphore,
                VkQueue graphics_queue_1,
                VkQueue graphics_queue_2,
                unsigned index,
                bool shadow_mapping,
                const TransparencyBuffers& transparency_buffers) const;
};
}
