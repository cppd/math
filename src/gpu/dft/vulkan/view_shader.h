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
#include <src/numerical/vec.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <unordered_set>
#include <vector>

namespace gpu
{
class DftViewMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int IMAGE_BINDING = 1;
        static constexpr int DATA_BINDING = 0;

        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

        struct Data
        {
                vec4f background_color;
                vec4f foreground_color;
                float brightness;
        };

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        DftViewMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::unordered_set<uint32_t>& family_indices);

        DftViewMemory(const DftViewMemory&) = delete;
        DftViewMemory& operator=(const DftViewMemory&) = delete;
        DftViewMemory& operator=(DftViewMemory&&) = delete;

        DftViewMemory(DftViewMemory&&) = default;
        ~DftViewMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_background_color(const vec4f& background_color) const;
        void set_foreground_color(const vec4f& foreground_color) const;
        void set_brightness(float brightness) const;
        void set_image(VkSampler sampler, const vulkan::ImageWithMemory& image) const;
};

struct DftViewVertex
{
        vec4f position;
        vec2f texture_coordinates;

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();
        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
};

class DftViewProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit DftViewProgram(const vulkan::Device& device);

        DftViewProgram(const DftViewProgram&) = delete;
        DftViewProgram& operator=(const DftViewProgram&) = delete;
        DftViewProgram& operator=(DftViewProgram&&) = delete;

        DftViewProgram(DftViewProgram&&) = default;
        ~DftViewProgram() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
