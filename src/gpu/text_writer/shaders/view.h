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

#include <src/color/color.h>
#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <unordered_set>
#include <vector>

namespace ns::gpu::text_writer
{
class Memory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int MATRICES_BINDING = 0;
        static constexpr int TEXTURE_BINDING = 1;
        static constexpr int DRAWING_BINDING = 2;

        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

        struct Matrices
        {
                mat4f matrix;
        };

        struct Drawing
        {
                vec3f color;
        };

        std::size_t m_matrices_buffer_index;
        std::size_t m_drawing_buffer_index;

        template <typename T>
        void copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const;
        template <typename T>
        void copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        Memory(const vulkan::Device& device,
               VkDescriptorSetLayout descriptor_set_layout,
               const std::unordered_set<uint32_t>& family_indices,
               VkSampler sampler,
               const vulkan::ImageWithMemory* texture);

        Memory(const Memory&) = delete;
        Memory& operator=(const Memory&) = delete;
        Memory& operator=(Memory&&) = delete;

        Memory(Memory&&) = default;
        ~Memory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_matrix(const mat4d& matrix) const;
        void set_color(const Color& color) const;
};

struct Vertex
{
        Vector<2, int32_t> window_coordinates;
        Vector<2, float> texture_coordinates;

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();
        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
};

class Program final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit Program(const vulkan::Device& device);

        Program(const Program&) = delete;
        Program& operator=(const Program&) = delete;
        Program& operator=(Program&&) = delete;

        Program(Program&&) = default;
        ~Program() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                bool sample_shading,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
