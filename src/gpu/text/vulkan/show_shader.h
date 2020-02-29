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

#include "../../com/glsl.h"

#include <src/color/color.h>
#include <src/numerical/matrix.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <unordered_set>
#include <vector>

namespace gpu_vulkan
{
class TextShowMemory final
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
                alignas(GLSL_VEC3_ALIGN) vec3f color;
        };

        size_t m_matrices_buffer_index;
        size_t m_drawing_buffer_index;

        template <typename T>
        void copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const;
        template <typename T>
        void copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        TextShowMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::unordered_set<uint32_t>& family_indices,
                VkSampler sampler,
                const vulkan::ImageWithMemory* texture);

        TextShowMemory(const TextShowMemory&) = delete;
        TextShowMemory& operator=(const TextShowMemory&) = delete;
        TextShowMemory& operator=(TextShowMemory&&) = delete;

        TextShowMemory(TextShowMemory&&) = default;
        ~TextShowMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_matrix(const mat4& matrix) const;
        void set_color(const Color& color) const;
};

struct TextShowVertex
{
        // Для данных используется TextVertex
        static std::vector<VkVertexInputBindingDescription> binding_descriptions();
        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
};

class TextShowProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit TextShowProgram(const vulkan::Device& device);

        TextShowProgram(const TextShowProgram&) = delete;
        TextShowProgram& operator=(const TextShowProgram&) = delete;
        TextShowProgram& operator=(TextShowProgram&&) = delete;

        TextShowProgram(TextShowProgram&&) = default;
        ~TextShowProgram() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                bool sample_shading,
                unsigned x,
                unsigned y,
                unsigned width,
                unsigned height) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
