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

#include "volume_object.h"

#include "shaders/volume.h"

#include <src/gpu/buffers.h>

#include <optional>
#include <thread>

namespace ns::gpu::renderer
{
class VolumeRenderer
{
        const std::thread::id m_thread_id = std::this_thread::get_id();
        const vulkan::Device& m_device;
        const bool m_sample_shading;

        const RenderBuffers3D* m_render_buffers = nullptr;

        VolumeProgram m_program;

        VolumeSharedMemory m_shared_memory;

        std::optional<vulkan::Pipeline> m_pipeline_image;
        std::optional<vulkan::Pipeline> m_pipeline_image_fragments;
        std::optional<vulkan::Pipeline> m_pipeline_fragments;
        std::optional<vulkan::CommandBuffers> m_command_buffers_image;
        std::optional<vulkan::CommandBuffers> m_command_buffers_image_fragments;
        std::optional<vulkan::CommandBuffers> m_command_buffers_fragments;

        vulkan::Sampler m_image_sampler;
        vulkan::Sampler m_depth_sampler;
        vulkan::Sampler m_transfer_function_sampler;

        void draw_commands_fragments(VkCommandBuffer command_buffer) const;
        void draw_commands_image(const VolumeObject* volume, VkCommandBuffer command_buffer) const;
        void draw_commands_image_fragments(const VolumeObject* volume, VkCommandBuffer command_buffer) const;

        void create_command_buffers_fragments(VkCommandPool graphics_command_pool);

public:
        VolumeRenderer(const vulkan::Device& device, bool sample_shading, const ShaderBuffers& buffers);

        std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts() const;
        VkSampler image_sampler() const;
        VkSampler transfer_function_sampler() const;

        void create_buffers(
                const RenderBuffers3D* render_buffers,
                VkCommandPool graphics_command_pool,
                const Region<2, int>& viewport,
                VkImageView depth_image,
                const vulkan::ImageWithMemory& transparency_heads_image,
                const vulkan::Buffer& transparency_nodes);
        void delete_buffers();

        void create_command_buffers(
                const VolumeObject* volume,
                VkCommandPool graphics_command_pool,
                const std::function<void(VkCommandBuffer command_buffer)>& before_render_pass_commands);
        void delete_command_buffers();

        bool has_volume() const;
        std::optional<VkCommandBuffer> command_buffer(unsigned index, bool with_fragments) const;
};
}
