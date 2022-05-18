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

#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::optical_flow
{
class ViewDataBuffer final
{
        struct Data final
        {
                Matrix4f matrix;
        };

        vulkan::BufferWithMemory buffer_;

public:
        ViewDataBuffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices);

        [[nodiscard]] const vulkan::Buffer& buffer() const;

        void set_matrix(const Matrix4d& matrix) const;
};

class ViewMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int POINTS_BINDING = 0;
        static constexpr int FLOW_BINDING = 1;
        static constexpr int DATA_BINDING = 2;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        ViewMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, const vulkan::Buffer& data_buffer);

        [[nodiscard]] const VkDescriptorSet& descriptor_set() const;

        void set_points(const vulkan::Buffer& buffer) const;
        void set_flow(const vulkan::Buffer& buffer) const;
};

class ViewProgram final
{
        const vulkan::Device* device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::Shader vertex_shader_;
        vulkan::Shader fragment_shader_;

public:
        explicit ViewProgram(const vulkan::Device* device);

        ViewProgram(const ViewProgram&) = delete;
        ViewProgram& operator=(const ViewProgram&) = delete;
        ViewProgram& operator=(ViewProgram&&) = delete;

        ViewProgram(ViewProgram&&) = default;
        ~ViewProgram() = default;

        [[nodiscard]] vulkan::handle::Pipeline create_pipeline(
                const vulkan::RenderPass& render_pass,
                VkSampleCountFlagBits sample_count,
                VkPrimitiveTopology primitive_topology,
                const Region<2, int>& viewport) const;

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const;
        [[nodiscard]] VkPipelineLayout pipeline_layout() const;
};
}
