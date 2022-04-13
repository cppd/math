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

#include "../../code/code.h"

#include <src/numerical/region.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::renderer
{
enum class VolumeProgramPipelineType
{
        FRAGMENTS,
        FRAGMENTS_OPACITY,
        IMAGE,
        IMAGE_FRAGMENTS,
        IMAGE_FRAGMENTS_OPACITY,
        IMAGE_OPACITY,
        OPACITY
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
        vulkan::Shader fragment_shader_fragments_;
        vulkan::Shader fragment_shader_fragments_opacity_;
        vulkan::Shader fragment_shader_image_;
        vulkan::Shader fragment_shader_image_fragments_;
        vulkan::Shader fragment_shader_image_fragments_opacity_;
        vulkan::Shader fragment_shader_image_opacity_;
        vulkan::Shader fragment_shader_opacity_;

        VkPipelineLayout pipeline_layout(VolumeProgramPipelineType type) const;
        const vulkan::Shader* fragment_shader(VolumeProgramPipelineType type) const;

public:
        explicit VolumeProgram(const vulkan::Device* device, const Code& code);

        VolumeProgram(const VolumeProgram&) = delete;
        VolumeProgram& operator=(const VolumeProgram&) = delete;
        VolumeProgram& operator=(VolumeProgram&&) = delete;

        VolumeProgram(VolumeProgram&&) = default;
        ~VolumeProgram() = default;

        vulkan::handle::Pipeline create_pipeline(
                const vulkan::RenderPass& render_pass,
                VkSampleCountFlagBits sample_count,
                bool sample_shading,
                const Region<2, int>& viewport,
                VolumeProgramPipelineType type) const;

        VkDescriptorSetLayout descriptor_set_layout_shared() const;
        std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_shared_bindings() const;

        VkDescriptorSetLayout descriptor_set_layout_image() const;
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_image_bindings();

        VkPipelineLayout pipeline_layout_shared() const;
        VkPipelineLayout pipeline_layout_shared_image() const;
};
}
