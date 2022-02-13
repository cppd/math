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

#pragma once

#include "object.h"

#include "../buffers/ggx_f1_albedo.h"
#include "../buffers/shader.h"
#include "../code/code.h"
#include "shaders/descriptors.h"
#include "shaders/program_volume.h"

#include <src/gpu/render_buffers.h>

#include <optional>
#include <thread>

namespace ns::gpu::renderer
{
class VolumeRenderer
{
        const std::thread::id thread_id_ = std::this_thread::get_id();
        const VkDevice device_;
        const bool sample_shading_;

        const RenderBuffers3D* render_buffers_ = nullptr;

        VolumeProgram program_;

        VolumeSharedMemory shared_memory_;

        std::optional<vulkan::handle::Pipeline> pipeline_image_;
        std::optional<vulkan::handle::Pipeline> pipeline_image_fragments_;
        std::optional<vulkan::handle::Pipeline> pipeline_fragments_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_image_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_image_fragments_;
        std::optional<vulkan::handle::CommandBuffers> command_buffers_fragments_;

        vulkan::handle::Sampler image_sampler_;
        vulkan::handle::Sampler depth_sampler_;
        vulkan::handle::Sampler transfer_function_sampler_;

        void draw_commands_fragments(VkCommandBuffer command_buffer) const;
        void draw_commands_image(const VolumeObject* volume, VkCommandBuffer command_buffer) const;
        void draw_commands_image_fragments(const VolumeObject* volume, VkCommandBuffer command_buffer) const;

        void create_command_buffers_fragments(VkCommandPool graphics_command_pool);

public:
        VolumeRenderer(
                const vulkan::Device* device,
                const Code& code,
                bool sample_shading,
                const DrawingBuffer& drawing_buffer,
                const GgxF1Albedo& ggx_f1_albedo);

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

        void set_acceleration_structure(VkAccelerationStructureKHR acceleration_structure);

        bool has_volume() const;
        std::optional<VkCommandBuffer> command_buffer(unsigned index, bool with_fragments) const;
};
}
