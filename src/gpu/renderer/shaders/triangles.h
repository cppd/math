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

#include <src/numerical/region.h>
#include <src/numerical/vec.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::renderer
{
class TrianglesMaterialMemory final
{
        static constexpr int SET_NUMBER = 2;

        static constexpr int MATERIAL_BINDING = 0;
        static constexpr int TEXTURE_BINDING = 1;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        struct MaterialInfo final
        {
                VkBuffer buffer;
                VkDeviceSize buffer_size;
                VkImageView texture;
        };

        static vulkan::Descriptors create(
                VkDevice device,
                VkSampler sampler,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const std::vector<MaterialInfo>& materials);
};

class TrianglesProgram final
{
        const vulkan::Device& device_;

        vulkan::DescriptorSetLayout descriptor_set_layout_shared_;
        vulkan::DescriptorSetLayout descriptor_set_layout_mesh_;
        vulkan::DescriptorSetLayout descriptor_set_layout_material_;
        vulkan::PipelineLayout pipeline_layout_;
        vulkan::VertexShader vertex_shader_;
        vulkan::GeometryShader geometry_shader_;
        vulkan::FragmentShader fragment_shader_;

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
                const Region<2, int>& viewport,
                bool transparency) const;

        VkDescriptorSetLayout descriptor_set_layout_shared() const;
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_shared_bindings();

        VkDescriptorSetLayout descriptor_set_layout_mesh() const;
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_mesh_bindings();

        VkDescriptorSetLayout descriptor_set_layout_material() const;
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_material_bindings();

        VkPipelineLayout pipeline_layout() const;
};
}
