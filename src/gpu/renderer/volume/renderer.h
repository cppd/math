/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "buffers/coordinates.h"
#include "shaders/descriptors.h"
#include "shaders/program_volume.h"

#include <src/gpu/render_buffers.h>
#include <src/gpu/renderer/buffers/ggx_f1_albedo.h>
#include <src/gpu/renderer/buffers/opacity.h>
#include <src/gpu/renderer/code/code.h>
#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

namespace ns::gpu::renderer
{
class VolumeRenderer final
{
        struct CommandsFragments final
        {
                vulkan::handle::CommandBuffers opacity;
                vulkan::handle::CommandBuffers opacity_transparency;
                vulkan::handle::CommandBuffers transparency;
        };

        struct CommandsImage final
        {
                vulkan::handle::CommandBuffers image;
                vulkan::handle::CommandBuffers image_opacity;
                vulkan::handle::CommandBuffers image_opacity_transparency;
                vulkan::handle::CommandBuffers image_transparency;
        };

        [[nodiscard]] static std::optional<VkCommandBuffer> commands(
                const CommandsFragments& commands,
                unsigned index,
                bool opacity,
                bool transparency);

        [[nodiscard]] static VkCommandBuffer commands(
                const CommandsImage& commands,
                unsigned index,
                bool opacity,
                bool transparency);

        const std::thread::id thread_id_ = std::this_thread::get_id();
        const VkDevice device_;
        const bool sample_shading_;

        const RenderBuffers3D* render_buffers_ = nullptr;

        VolumeCoordinatesBuffer coordinates_buffer_;

        VolumeProgram volume_program_;

        VolumeSharedMemory shared_memory_;

        std::unordered_map<VolumeProgramPipelineType, vulkan::handle::Pipeline> pipelines_;

        std::optional<CommandsFragments> commands_fragments_;
        std::optional<CommandsImage> commands_image_;

        vulkan::handle::Sampler image_sampler_;
        vulkan::handle::Sampler depth_sampler_;
        vulkan::handle::Sampler transfer_function_sampler_;

        void draw_commands_fragments(VolumeProgramPipelineType type, VkCommandBuffer command_buffer) const;

        void draw_commands_image(
                VolumeProgramPipelineType type,
                const VolumeObject* volume,
                VkCommandBuffer command_buffer) const;

        void create_command_buffers_fragments(VkCommandPool graphics_command_pool);

        void create_command_buffers_image(
                const VolumeObject* volume,
                VkCommandPool graphics_command_pool,
                const std::function<void(VkCommandBuffer command_buffer)>& before_render_pass_commands);

public:
        VolumeRenderer(
                const vulkan::Device* device,
                const Code& code,
                bool sample_shading,
                const std::vector<std::uint32_t>& graphics_family_indices,
                const vulkan::Buffer& drawing_buffer,
                const GgxF1Albedo& ggx_f1_albedo);

        [[nodiscard]] std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts() const;
        [[nodiscard]] VkSampler image_sampler() const;
        [[nodiscard]] VkSampler transfer_function_sampler() const;

        void create_buffers(
                const RenderBuffers3D* render_buffers,
                const numerical::Region<2, int>& viewport,
                VkImageView depth_image,
                const vulkan::ImageWithMemory& transparency_heads_image,
                const vulkan::Buffer& transparency_nodes,
                const Opacity& opacity);

        void delete_buffers();

        void create_command_buffers(VkCommandPool graphics_command_pool);

        void create_command_buffers(
                const VolumeObject* volume,
                VkCommandPool graphics_command_pool,
                const std::function<void(VkCommandBuffer command_buffer)>& before_render_pass_commands);

        void delete_command_buffers();

        void set_shadow_image(VkSampler sampler, const vulkan::ImageView& shadow_image);
        void set_acceleration_structure(VkAccelerationStructureKHR acceleration_structure);

        [[nodiscard]] bool has_volume() const;
        [[nodiscard]] std::optional<VkCommandBuffer> command_buffer(unsigned index, bool opacity, bool transparency)
                const;

        void set_matrix(const numerical::Matrix4d& vp_matrix);
        void set_matrix(const numerical::Matrix4d& vp_matrix, const numerical::Matrix4d& world_to_shadow_matrix);
};
}
