/*
Copyright (C) 2017-2023 Topological Manifold

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
#include <src/vulkan/device/device.h>
#include <src/vulkan/layout.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <cstdint>
#include <vector>

namespace ns::gpu::text_writer
{
class Buffer final
{
        struct Data final
        {
                vulkan::std140::Matrix4f matrix;
                vulkan::std140::Vector3f color;
        };

        vulkan::BufferWithMemory buffer_;

public:
        Buffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices);

        [[nodiscard]] const vulkan::Buffer& buffer() const;

        void set_matrix(const Matrix4d& matrix) const;
        void set_color(const Vector3f& color) const;
};

class Memory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DATA_BINDING = 0;
        static constexpr int TEXTURE_BINDING = 1;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        Memory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout, const vulkan::Buffer& buffer);

        void set_image(VkSampler sampler, VkImageView image) const;

        [[nodiscard]] const VkDescriptorSet& descriptor_set() const;
};

struct Vertex final
{
        Vector<2, std::int32_t> window_coordinates;
        Vector<2, float> texture_coordinates;

        [[nodiscard]] static std::vector<VkVertexInputBindingDescription> binding_descriptions();
        [[nodiscard]] static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
};

class Program final
{
        const vulkan::Device* device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::Shader vertex_shader_;
        vulkan::Shader fragment_shader_;

public:
        explicit Program(const vulkan::Device* device);

        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;
        Program& operator=(Program&&) = delete;

        Program(Program&&) = default;
        ~Program() = default;

        [[nodiscard]] vulkan::handle::Pipeline create_pipeline(
                const vulkan::RenderPass& render_pass,
                VkSampleCountFlagBits sample_count,
                bool sample_shading,
                const Region<2, int>& viewport) const;

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const;
        [[nodiscard]] VkPipelineLayout pipeline_layout() const;
};
}
