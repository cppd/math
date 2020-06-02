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

#include "buffers.h"

#include <src/numerical/region.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace gpu::renderer
{
class NormalsSharedMemory final
{
        static constexpr int SET_NUMBER = 0;
        static constexpr int MATRICES_BINDING = 0;
        static constexpr int DRAWING_BINDING = 1;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        NormalsSharedMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const vulkan::Buffer& matrices,
                const vulkan::Buffer& drawing);

        NormalsSharedMemory(const NormalsSharedMemory&) = delete;
        NormalsSharedMemory& operator=(const NormalsSharedMemory&) = delete;
        NormalsSharedMemory& operator=(NormalsSharedMemory&&) = delete;

        NormalsSharedMemory(NormalsSharedMemory&&) = default;
        ~NormalsSharedMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;
};

class NormalsMeshMemory final
{
        static constexpr int SET_NUMBER = 1;
        static constexpr int BUFFER_BINDING = 0;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        static vulkan::Descriptors create(
                VkDevice device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<CoordinatesInfo>& coordinates);
};

class NormalsProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout_shared;
        vulkan::DescriptorSetLayout m_descriptor_set_layout_mesh;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::GeometryShader m_geometry_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit NormalsProgram(const vulkan::Device& device);

        NormalsProgram(const NormalsProgram&) = delete;
        NormalsProgram& operator=(const NormalsProgram&) = delete;
        NormalsProgram& operator=(NormalsProgram&&) = delete;

        NormalsProgram(NormalsProgram&&) = default;
        ~NormalsProgram() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                bool sample_shading,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout_shared() const;
        VkDescriptorSetLayout descriptor_set_layout_mesh() const;

        VkPipelineLayout pipeline_layout() const;
};
}
