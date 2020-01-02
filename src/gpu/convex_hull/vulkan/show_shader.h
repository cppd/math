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
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"

#include <unordered_set>
#include <vector>

namespace gpu_vulkan
{
class ConvexHullShowMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DATA_BINDING = 0;
        static constexpr int POINTS_BINDING = 1;

        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

        struct Data
        {
                Matrix<4, 4, float> matrix;
                float brightness;
        };

        size_t m_data_buffer_index;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        ConvexHullShowMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout,
                             const std::unordered_set<uint32_t>& family_indices);

        ConvexHullShowMemory(const ConvexHullShowMemory&) = delete;
        ConvexHullShowMemory& operator=(const ConvexHullShowMemory&) = delete;
        ConvexHullShowMemory& operator=(ConvexHullShowMemory&&) = delete;

        ConvexHullShowMemory(ConvexHullShowMemory&&) = default;
        ~ConvexHullShowMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_matrix(const mat4& matrix) const;
        void set_brightness(float brightness) const;
        void set_points(const vulkan::BufferWithMemory& buffer) const;
};

class ConvexHullShowProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        ConvexHullShowProgram(const vulkan::Device& device);

        ConvexHullShowProgram(const ConvexHullShowProgram&) = delete;
        ConvexHullShowProgram& operator=(const ConvexHullShowProgram&) = delete;
        ConvexHullShowProgram& operator=(ConvexHullShowProgram&&) = delete;

        ConvexHullShowProgram(ConvexHullShowProgram&&) = default;
        ~ConvexHullShowProgram() = default;

        vulkan::Pipeline create_pipeline(VkRenderPass render_pass, VkSampleCountFlagBits sample_count, bool sample_shading,
                                         unsigned x, unsigned y, unsigned width, unsigned height) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
