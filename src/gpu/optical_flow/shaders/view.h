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

#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/numerical/vec.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <unordered_set>
#include <vector>

namespace ns::gpu::optical_flow
{
class ViewMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int POINTS_BINDING = 0;
        static constexpr int FLOW_BINDING = 1;
        static constexpr int DATA_BINDING = 2;

        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

        struct Data
        {
                mat4f matrix;
        };

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        ViewMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::unordered_set<uint32_t>& family_indices);

        ViewMemory(const ViewMemory&) = delete;
        ViewMemory& operator=(const ViewMemory&) = delete;
        ViewMemory& operator=(ViewMemory&&) = delete;

        ViewMemory(ViewMemory&&) = default;
        ~ViewMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_points(const vulkan::BufferWithMemory& buffer) const;
        void set_flow(const vulkan::BufferWithMemory& buffer) const;
        void set_matrix(const mat4d& matrix) const;
};

class ViewProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit ViewProgram(const vulkan::Device& device);

        ViewProgram(const ViewProgram&) = delete;
        ViewProgram& operator=(const ViewProgram&) = delete;
        ViewProgram& operator=(ViewProgram&&) = delete;

        ViewProgram(ViewProgram&&) = default;
        ~ViewProgram() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                VkPrimitiveTopology primitive_topology,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
