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

namespace ns::gpu::text_writer
{
class Buffer final
{
        struct Data final
        {
                alignas(sizeof(Vector4f)) Matrix4f matrix;
                alignas(sizeof(Vector4f)) Vector3f color;
        };

        vulkan::BufferWithMemory buffer_;

public:
        Buffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices);

        const vulkan::Buffer& buffer() const;

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
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        Memory(const vulkan::Device& device,
               VkDescriptorSetLayout descriptor_set_layout,
               const vulkan::Buffer& data_buffer,
               VkSampler sampler,
               VkImageView texture);

        const VkDescriptorSet& descriptor_set() const;
};

struct Vertex final
{
        Vector<2, std::int32_t> window_coordinates;
        Vector<2, float> texture_coordinates;

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();
        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
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

        vulkan::handle::Pipeline create_pipeline(
                const vulkan::RenderPass& render_pass,
                VkSampleCountFlagBits sample_count,
                bool sample_shading,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
