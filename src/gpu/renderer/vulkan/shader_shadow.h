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

#include "shader_buffers.h"

#include <src/numerical/region.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace gpu_vulkan
{
class RendererShadowMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int MATRICES_BINDING = 0;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        RendererShadowMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const RendererBuffers& buffers);

        RendererShadowMemory(const RendererShadowMemory&) = delete;
        RendererShadowMemory& operator=(const RendererShadowMemory&) = delete;
        RendererShadowMemory& operator=(RendererShadowMemory&&) = delete;

        RendererShadowMemory(RendererShadowMemory&&) = default;
        ~RendererShadowMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;
};

class RendererShadowProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit RendererShadowProgram(const vulkan::Device& device);

        RendererShadowProgram(const RendererShadowProgram&) = delete;
        RendererShadowProgram& operator=(const RendererShadowProgram&) = delete;
        RendererShadowProgram& operator=(RendererShadowProgram&&) = delete;

        RendererShadowProgram(RendererShadowProgram&&) = default;
        ~RendererShadowProgram() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                const Region<2, int>& rectangle) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
