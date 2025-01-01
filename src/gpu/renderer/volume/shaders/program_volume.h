/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/gpu/renderer/code/code.h>
#include <src/numerical/region.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vulkan/vulkan_core.h>

#include <vector>

namespace ns::gpu::renderer
{
enum class VolumeProgramPipelineType
{
        IMAGE,
        IMAGE_OPACITY,
        IMAGE_OPACITY_TRANSPARENCY,
        IMAGE_TRANSPARENCY,
        OPACITY,
        OPACITY_TRANSPARENCY,
        TRANSPARENCY
};

class VolumeProgram final
{
        const vulkan::Device* device_;
        bool ray_tracing_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_shared_;
        vulkan::handle::DescriptorSetLayout descriptor_set_layout_image_;
        vulkan::handle::PipelineLayout pipeline_layout_shared_image_;
        vulkan::handle::PipelineLayout pipeline_layout_shared_;
        vulkan::Shader vertex_shader_;
        vulkan::Shader fragment_shader_image_;
        vulkan::Shader fragment_shader_image_opacity_;
        vulkan::Shader fragment_shader_image_opacity_transparency_;
        vulkan::Shader fragment_shader_image_transparency_;
        vulkan::Shader fragment_shader_opacity_;
        vulkan::Shader fragment_shader_opacity_transparency_;
        vulkan::Shader fragment_shader_transparency_;

        [[nodiscard]] VkPipelineLayout pipeline_layout(VolumeProgramPipelineType type) const;
        [[nodiscard]] const vulkan::Shader* fragment_shader(VolumeProgramPipelineType type) const;

public:
        explicit VolumeProgram(const vulkan::Device* device, const Code& code);

        VolumeProgram(const VolumeProgram&) = delete;
        VolumeProgram& operator=(const VolumeProgram&) = delete;
        VolumeProgram& operator=(VolumeProgram&&) = delete;

        VolumeProgram(VolumeProgram&&) = default;
        ~VolumeProgram() = default;

        [[nodiscard]] vulkan::handle::Pipeline create_pipeline(
                const vulkan::RenderPass& render_pass,
                VkSampleCountFlagBits sample_count,
                bool sample_shading,
                const numerical::Region<2, int>& viewport,
                VolumeProgramPipelineType type) const;

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout_shared() const;
        [[nodiscard]] std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_shared_bindings() const;

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout_image() const;
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_image_bindings();

        [[nodiscard]] VkPipelineLayout pipeline_layout_shared() const;
        [[nodiscard]] VkPipelineLayout pipeline_layout_shared_image() const;
};
}
