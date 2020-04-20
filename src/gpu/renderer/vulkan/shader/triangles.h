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
#include <src/numerical/vec.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <unordered_set>
#include <vector>

namespace gpu::renderer
{
class TrianglesMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int MATRICES_BINDING = 0;
        static constexpr int LIGHTING_BINDING = 1;
        static constexpr int DRAWING_BINDING = 2;
        static constexpr int SHADOW_BINDING = 3;
        static constexpr int OBJECTS_BINDING = 4;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        TrianglesMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const vulkan::Buffer& matrices,
                const vulkan::Buffer& lighting,
                const vulkan::Buffer& drawing);

        TrianglesMemory(const TrianglesMemory&) = delete;
        TrianglesMemory& operator=(const TrianglesMemory&) = delete;
        TrianglesMemory& operator=(TrianglesMemory&&) = delete;

        TrianglesMemory(TrianglesMemory&&) = default;
        ~TrianglesMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_shadow_texture(VkSampler sampler, const vulkan::DepthAttachment* shadow_texture) const;
        void set_object_image(const vulkan::ImageWithMemory* storage_image) const;
};

class TrianglesMaterialMemory final
{
        static constexpr int SET_NUMBER = 1;

        static constexpr int MATERIAL_BINDING = 0;
        static constexpr int TEXTURE_KA_BINDING = 1;
        static constexpr int TEXTURE_KD_BINDING = 2;
        static constexpr int TEXTURE_KS_BINDING = 3;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        static vulkan::Descriptors create(
                VkDevice device,
                VkSampler sampler,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<MaterialInfo>& materials);
};

class TrianglesProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::DescriptorSetLayout m_descriptor_set_layout_material;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::GeometryShader m_geometry_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit TrianglesProgram(const vulkan::Device& device);

        TrianglesProgram(const TrianglesProgram&) = delete;
        TrianglesProgram& operator=(const TrianglesProgram&) = delete;
        TrianglesProgram& operator=(TrianglesProgram&&) = delete;

        TrianglesProgram(TrianglesProgram&&) = default;
        ~TrianglesProgram() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                bool sample_shading,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkDescriptorSetLayout descriptor_set_layout_material() const;
        VkPipelineLayout pipeline_layout() const;
};
}
