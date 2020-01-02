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

#include "com/matrix.h"
#include "com/vec.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"

#include <unordered_set>
#include <vector>

namespace gpu_vulkan
{
class DftShowMemory final
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

        DftShowMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout,
                      const std::unordered_set<uint32_t>& family_indices);

        DftShowMemory(const DftShowMemory&) = delete;
        DftShowMemory& operator=(const DftShowMemory&) = delete;
        DftShowMemory& operator=(DftShowMemory&&) = delete;

        DftShowMemory(DftShowMemory&&) = default;
        ~DftShowMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_background_color(const vec4f& background_color) const;
        void set_foreground_color(const vec4f& foreground_color) const;
        void set_brightness(float brightness) const;
        void set_image(VkSampler sampler, const vulkan::ImageWithMemory& image) const;
};

struct DftShowVertex
{
        vec4f position;
        vec2f texture_coordinates;

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();
        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
};

class DftShowProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        DftShowProgram(const vulkan::Device& device);

        DftShowProgram(const DftShowProgram&) = delete;
        DftShowProgram& operator=(const DftShowProgram&) = delete;
        DftShowProgram& operator=(DftShowProgram&&) = delete;

        DftShowProgram(DftShowProgram&&) = default;
        ~DftShowProgram() = default;

        vulkan::Pipeline create_pipeline(VkRenderPass render_pass, VkSampleCountFlagBits sample_count, unsigned x, unsigned y,
                                         unsigned width, unsigned height) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
