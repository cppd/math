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

#include <src/color/color.h>
#include <src/numerical/region.h>
#include <src/numerical/vec.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace gpu
{
class RendererPointsMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int MATRICES_BINDING = 0;
        static constexpr int DRAWING_BINDING = 1;
        static constexpr int OBJECTS_BINDING = 2;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        RendererPointsMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const RendererBuffers& buffers);

        RendererPointsMemory(const RendererPointsMemory&) = delete;
        RendererPointsMemory& operator=(const RendererPointsMemory&) = delete;
        RendererPointsMemory& operator=(RendererPointsMemory&&) = delete;

        RendererPointsMemory(RendererPointsMemory&&) = default;
        ~RendererPointsMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_object_image(const vulkan::ImageWithMemory* storage_image) const;
};

struct RendererPointsVertex
{
        vec3f position;

        explicit constexpr RendererPointsVertex(const vec3f& position_) : position(position_)
        {
        }

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();

        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
};

class RendererPointsProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader_0d;
        vulkan::VertexShader m_vertex_shader_1d;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit RendererPointsProgram(const vulkan::Device& device);

        RendererPointsProgram(const RendererPointsProgram&) = delete;
        RendererPointsProgram& operator=(const RendererPointsProgram&) = delete;
        RendererPointsProgram& operator=(RendererPointsProgram&&) = delete;

        RendererPointsProgram(RendererPointsProgram&&) = default;
        ~RendererPointsProgram() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                VkPrimitiveTopology primitive_topology,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
