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

#include "triangles.h"

#include "descriptors.h"
#include "vertex_triangles.h"

#include "../code/code.h"

#include <src/com/error.h>
#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> TrianglesMaterialMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = MATERIAL_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TEXTURE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

vulkan::Descriptors TrianglesMaterialMemory::create(
        VkDevice device,
        VkSampler sampler,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const std::vector<MaterialInfo>& materials)
{
        ASSERT(!materials.empty());
        ASSERT(std::all_of(
                materials.cbegin(), materials.cend(),
                [](const MaterialInfo& m)
                {
                        return m.buffer != VK_NULL_HANDLE && m.buffer_size > 0 && m.texture != VK_NULL_HANDLE;
                }));

        vulkan::Descriptors descriptors(
                vulkan::Descriptors(device, materials.size(), descriptor_set_layout, descriptor_set_layout_bindings));

        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        for (std::size_t i = 0; i < materials.size(); ++i)
        {
                const MaterialInfo& material = materials[i];

                infos.clear();
                bindings.clear();
                {
                        VkDescriptorBufferInfo buffer_info = {};
                        buffer_info.buffer = material.buffer;
                        buffer_info.offset = 0;
                        buffer_info.range = material.buffer_size;

                        infos.emplace_back(buffer_info);

                        bindings.push_back(MATERIAL_BINDING);
                }
                {
                        VkDescriptorImageInfo image_info = {};
                        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        image_info.imageView = material.texture;
                        image_info.sampler = sampler;

                        infos.emplace_back(image_info);

                        bindings.push_back(TEXTURE_BINDING);
                }
                descriptors.update_descriptor_set(i, bindings, infos);
        }

        return descriptors;
}

unsigned TrianglesMaterialMemory::set_number()
{
        return SET_NUMBER;
}

//

std::vector<VkDescriptorSetLayoutBinding> TrianglesProgram::descriptor_set_layout_shared_bindings()
{
        return CommonMemory::descriptor_set_layout_bindings(
                VK_SHADER_STAGE_VERTEX_BIT,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_STAGE_FRAGMENT_BIT);
}

std::vector<VkDescriptorSetLayoutBinding> TrianglesProgram::descriptor_set_layout_mesh_bindings()
{
        return MeshMemory::descriptor_set_layout_bindings(VK_SHADER_STAGE_VERTEX_BIT);
}

std::vector<VkDescriptorSetLayoutBinding> TrianglesProgram::descriptor_set_layout_material_bindings()
{
        return TrianglesMaterialMemory::descriptor_set_layout_bindings();
}

TrianglesProgram::TrianglesProgram(const vulkan::Device* device)
        : device_(device),
          descriptor_set_layout_shared_(
                  vulkan::create_descriptor_set_layout(*device, descriptor_set_layout_shared_bindings())),
          descriptor_set_layout_mesh_(
                  vulkan::create_descriptor_set_layout(*device, descriptor_set_layout_mesh_bindings())),
          descriptor_set_layout_material_(
                  vulkan::create_descriptor_set_layout(*device, descriptor_set_layout_material_bindings())),
          pipeline_layout_(vulkan::create_pipeline_layout(
                  *device,
                  {CommonMemory::set_number(), MeshMemory::set_number(), TrianglesMaterialMemory::set_number()},
                  {descriptor_set_layout_shared_, descriptor_set_layout_mesh_, descriptor_set_layout_material_})),
          vertex_shader_(*device_, code_triangles_vert(), "main"),
          geometry_shader_(*device_, code_triangles_geom(), "main"),
          fragment_shader_(*device_, code_triangles_frag(), "main")
{
}

VkDescriptorSetLayout TrianglesProgram::descriptor_set_layout_shared() const
{
        return descriptor_set_layout_shared_;
}
VkDescriptorSetLayout TrianglesProgram::descriptor_set_layout_mesh() const
{
        return descriptor_set_layout_mesh_;
}

VkDescriptorSetLayout TrianglesProgram::descriptor_set_layout_material() const
{
        return descriptor_set_layout_material_;
}

VkPipelineLayout TrianglesProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

vulkan::handle::Pipeline TrianglesProgram::create_pipeline(
        VkRenderPass render_pass,
        VkSampleCountFlagBits sample_count,
        bool sample_shading,
        const Region<2, int>& viewport,
        bool transparency) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = device_;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = sample_shading;
        info.pipeline_layout = pipeline_layout_;
        info.viewport = viewport;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        CommonConstants common_constants;
        common_constants.set(transparency);
        info.depth_write = !transparency;

        const std::vector<const vulkan::Shader*> shaders = {&vertex_shader_, &geometry_shader_, &fragment_shader_};
        const std::vector<const vulkan::SpecializationConstant*> constants = {
                &common_constants, &common_constants, &common_constants};
        const std::vector<VkVertexInputBindingDescription> binding_descriptions =
                TrianglesVertex::binding_descriptions();
        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                TrianglesVertex::attribute_descriptions_triangles();

        info.shaders = &shaders;
        info.constants = &constants;
        info.binding_descriptions = &binding_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
