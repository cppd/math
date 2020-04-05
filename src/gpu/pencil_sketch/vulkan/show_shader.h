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

#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <unordered_set>
#include <vector>

namespace gpu_vulkan
{
class PencilSketchShowMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int IMAGE_BINDING = 0;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        PencilSketchShowMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout);

        PencilSketchShowMemory(const PencilSketchShowMemory&) = delete;
        PencilSketchShowMemory& operator=(const PencilSketchShowMemory&) = delete;
        PencilSketchShowMemory& operator=(PencilSketchShowMemory&&) = delete;

        PencilSketchShowMemory(PencilSketchShowMemory&&) = default;
        ~PencilSketchShowMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_image(VkSampler sampler, const vulkan::ImageWithMemory& image) const;
};

struct PencilSketchShowVertex
{
        vec4f position;
        vec2f texture_coordinates;

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();
        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
};

class PencilSketchShowProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit PencilSketchShowProgram(const vulkan::Device& device);

        PencilSketchShowProgram(const PencilSketchShowProgram&) = delete;
        PencilSketchShowProgram& operator=(const PencilSketchShowProgram&) = delete;
        PencilSketchShowProgram& operator=(PencilSketchShowProgram&&) = delete;

        PencilSketchShowProgram(PencilSketchShowProgram&&) = default;
        ~PencilSketchShowProgram() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
