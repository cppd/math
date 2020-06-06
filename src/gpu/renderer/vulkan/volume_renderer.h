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

#pragma once

#include "volume_object.h"

#include "../../vulkan_interfaces.h"
#include "shader/buffers.h"
#include "shader/volume.h"

#include <optional>
#include <thread>

namespace gpu::renderer
{
class VolumeRenderer
{
        const std::thread::id m_thread_id = std::this_thread::get_id();
        const vulkan::Device& m_device;
        const bool m_sample_shading;

        const RenderBuffers3D* m_render_buffers = nullptr;

        VolumeProgram m_program;
        VolumeMemory m_memory;

        std::optional<vulkan::Pipeline> m_pipeline;
        std::optional<vulkan::CommandBuffers> m_command_buffers;

        vulkan::Sampler m_volume_sampler;

        void draw_commands(const VolumeObject* volume, VkCommandBuffer command_buffer) const;

public:
        VolumeRenderer(const vulkan::Device& device, bool sample_shading, const ShaderBuffers& buffers);

        vulkan::Descriptors create_volume_memory(const VolumeInfo& volume_info);

        void create_buffers(const RenderBuffers3D* render_buffers, const Region<2, int>& viewport);
        void delete_buffers();

        void create_command_buffers(
                const VolumeObject* volume,
                VkCommandPool graphics_command_pool,
                const Color& clear_color,
                const std::function<void(VkCommandBuffer command_buffer)>& before_render_pass_commands);
        void delete_command_buffers();

        std::optional<VkCommandBuffer> command_buffer(unsigned index) const;
};
}
