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

#include "program_volume.h"

#include "descriptors.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> VolumeProgram::descriptor_set_layout_shared_bindings(
        const bool fragments_only) const
{
        VolumeSharedMemory::Flags flags{};

        flags.drawing = VK_SHADER_STAGE_FRAGMENT_BIT;
        flags.coordinates = VK_SHADER_STAGE_FRAGMENT_BIT;
        flags.depth_image = fragments_only ? 0 : VK_SHADER_STAGE_FRAGMENT_BIT;
        flags.ggx_f1_albedo = VK_SHADER_STAGE_FRAGMENT_BIT;
        flags.shadow_map = !ray_tracing_ ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
        flags.acceleration_structure = ray_tracing_ ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;

        return VolumeSharedMemory::descriptor_set_layout_bindings(flags);
}

std::vector<VkDescriptorSetLayoutBinding> VolumeProgram::descriptor_set_layout_image_bindings()
{
        return VolumeImageMemory::descriptor_set_layout_bindings();
}

VolumeProgram::VolumeProgram(const vulkan::Device* const device, const Code& code)
        : device_(device),
          ray_tracing_(code.ray_tracing()),
          descriptor_set_layout_shared_image_(
                  vulkan::create_descriptor_set_layout(*device, descriptor_set_layout_shared_bindings(false))),
          descriptor_set_layout_shared_fragments_(
                  vulkan::create_descriptor_set_layout(*device, descriptor_set_layout_shared_bindings(true))),
          descriptor_set_layout_image_(
                  vulkan::create_descriptor_set_layout(*device, descriptor_set_layout_image_bindings())),
          pipeline_layout_image_(vulkan::create_pipeline_layout(
                  *device,
                  {VolumeSharedMemory::set_number(), VolumeImageMemory::set_number()},
                  {descriptor_set_layout_shared_image_, descriptor_set_layout_image_})),
          pipeline_layout_fragments_(vulkan::create_pipeline_layout(
                  *device,
                  {VolumeSharedMemory::set_number()},
                  {descriptor_set_layout_shared_fragments_})),
          vertex_shader_(*device_, code.volume_vert(), VK_SHADER_STAGE_VERTEX_BIT),
          fragment_shader_image_(*device_, code.volume_image_frag(), VK_SHADER_STAGE_FRAGMENT_BIT),
          fragment_shader_image_fragments_(*device_, code.volume_image_fragments_frag(), VK_SHADER_STAGE_FRAGMENT_BIT),
          fragment_shader_fragments_(*device_, code.volume_fragments_frag(), VK_SHADER_STAGE_FRAGMENT_BIT)
{
}

VkDescriptorSetLayout VolumeProgram::descriptor_set_layout_shared(const bool fragments_only) const
{
        if (fragments_only)
        {
                return descriptor_set_layout_shared_fragments_;
        }
        return descriptor_set_layout_shared_image_;
}

VkDescriptorSetLayout VolumeProgram::descriptor_set_layout_image() const
{
        return descriptor_set_layout_image_;
}

VkPipelineLayout VolumeProgram::pipeline_layout(const bool fragments_only) const
{
        if (fragments_only)
        {
                return pipeline_layout_fragments_;
        }
        return pipeline_layout_image_;
}

VkPipelineLayout VolumeProgram::pipeline_layout(const VolumeProgramPipelineType type) const
{
        switch (type)
        {
        case VolumeProgramPipelineType::IMAGE:
        case VolumeProgramPipelineType::IMAGE_FRAGMENTS:
                return pipeline_layout_image_;
        case VolumeProgramPipelineType::FRAGMENTS:
                return pipeline_layout_fragments_;
        }
        error_fatal("Unknown volume program pipeline type " + to_string(enum_to_int(type)));
}

const vulkan::Shader* VolumeProgram::fragment_shader(const VolumeProgramPipelineType type) const
{
        switch (type)
        {
        case VolumeProgramPipelineType::IMAGE:
                return &fragment_shader_image_;
        case VolumeProgramPipelineType::IMAGE_FRAGMENTS:
                return &fragment_shader_image_fragments_;
        case VolumeProgramPipelineType::FRAGMENTS:
                return &fragment_shader_fragments_;
        }
        error_fatal("Unknown volume program pipeline type " + to_string(enum_to_int(type)));
}

vulkan::handle::Pipeline VolumeProgram::create_pipeline(
        const vulkan::RenderPass& render_pass,
        const VkSampleCountFlagBits sample_count,
        const bool sample_shading,
        const Region<2, int>& viewport,
        const VolumeProgramPipelineType type) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = device_;
        info.render_pass = &render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = sample_shading;
        info.pipeline_layout = pipeline_layout(type);
        info.viewport = viewport;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        info.depth_test = false;
        info.depth_write = false;

        ASSERT(render_pass.color_attachment_count() == 1);
        VkPipelineColorBlendAttachmentState& state = info.color_blend.emplace_back();
        state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                               | VK_COLOR_COMPONENT_A_BIT;
        state.blendEnable = VK_TRUE;
        state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        state.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        state.colorBlendOp = VK_BLEND_OP_ADD;
        state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        state.alphaBlendOp = VK_BLEND_OP_ADD;

        const std::vector<const vulkan::Shader*> shaders{&vertex_shader_, fragment_shader(type)};
        info.shaders = &shaders;

        return vulkan::create_graphics_pipeline(info);
}
}
