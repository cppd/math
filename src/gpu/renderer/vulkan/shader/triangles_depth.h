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

#include <src/numerical/region.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace gpu
{
class RendererTrianglesDepthMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int MATRICES_BINDING = 0;
        static constexpr int DRAWING_BINDING = 1;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        RendererTrianglesDepthMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const vulkan::Buffer& matrices,
                const vulkan::Buffer& drawing);

        RendererTrianglesDepthMemory(const RendererTrianglesDepthMemory&) = delete;
        RendererTrianglesDepthMemory& operator=(const RendererTrianglesDepthMemory&) = delete;
        RendererTrianglesDepthMemory& operator=(RendererTrianglesDepthMemory&&) = delete;

        RendererTrianglesDepthMemory(RendererTrianglesDepthMemory&&) = default;
        ~RendererTrianglesDepthMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;
};

class RendererTrianglesDepthProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;

public:
        explicit RendererTrianglesDepthProgram(const vulkan::Device& device);

        RendererTrianglesDepthProgram(const RendererTrianglesDepthProgram&) = delete;
        RendererTrianglesDepthProgram& operator=(const RendererTrianglesDepthProgram&) = delete;
        RendererTrianglesDepthProgram& operator=(RendererTrianglesDepthProgram&&) = delete;

        RendererTrianglesDepthProgram(RendererTrianglesDepthProgram&&) = default;
        ~RendererTrianglesDepthProgram() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
