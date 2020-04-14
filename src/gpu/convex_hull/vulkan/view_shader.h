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
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <unordered_set>
#include <vector>

namespace gpu
{
class ConvexHullViewMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DATA_BINDING = 0;
        static constexpr int POINTS_BINDING = 1;

        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

        struct Data
        {
                mat4f matrix;
                float brightness;
        };

        size_t m_data_buffer_index;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        ConvexHullViewMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::unordered_set<uint32_t>& family_indices);

        ConvexHullViewMemory(const ConvexHullViewMemory&) = delete;
        ConvexHullViewMemory& operator=(const ConvexHullViewMemory&) = delete;
        ConvexHullViewMemory& operator=(ConvexHullViewMemory&&) = delete;

        ConvexHullViewMemory(ConvexHullViewMemory&&) = default;
        ~ConvexHullViewMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_matrix(const mat4& matrix) const;
        void set_brightness(float brightness) const;
        void set_points(const vulkan::BufferWithMemory& buffer) const;
};

class ConvexHullViewProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit ConvexHullViewProgram(const vulkan::Device& device);

        ConvexHullViewProgram(const ConvexHullViewProgram&) = delete;
        ConvexHullViewProgram& operator=(const ConvexHullViewProgram&) = delete;
        ConvexHullViewProgram& operator=(ConvexHullViewProgram&&) = delete;

        ConvexHullViewProgram(ConvexHullViewProgram&&) = default;
        ~ConvexHullViewProgram() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                bool sample_shading,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
